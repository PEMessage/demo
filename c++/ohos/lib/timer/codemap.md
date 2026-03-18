# OpenHarmony Timer Library - CodeMap

## 1. Responsibility: Timer/Timeout Management

The Timer library provides a high-level, thread-safe timer management system for OpenHarmony. It enables applications to register timed callbacks (both one-shot and periodic) with millisecond precision.

### Core Capabilities
- **Timer Registration**: Register callbacks with specified intervals (one-shot or repeating)
- **Timer Lifecycle Management**: Setup, run, and shutdown timers with proper resource cleanup
- **Event-Driven Architecture**: Uses epoll-based I/O multiplexing for efficient timer event handling
- **Thread Safety**: Mutex-protected data structures for concurrent access

### Key Classes
| Class | Responsibility |
|-------|---------------|
| `Timer` | Public API for timer management; manages timer entries and user callbacks |
| `EventReactor` | Core event loop controller; manages timer scheduling and lifecycle |
| `TimerEventHandler` | Wraps Linux timerfd; handles timer initialization and timeout events |
| `EventHandler` | Base class for event handlers; provides callback infrastructure |
| `EventDemultiplexer` | epoll wrapper; handles I/O event polling and dispatching |

---

## 2. Design: Timer Implementation Patterns

### 2.1 Architecture Pattern: Reactor Pattern

```
┌─────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│     Timer       │────▶│  EventReactor    │────▶│EventDemultiplexer│
│  (User API)     │     │ (Event Loop Ctrl)│     │   (epoll Wrapper)│
└─────────────────┘     └──────────────────┘     └──────────────────┘
         │                       │                          │
         ▼                       ▼                          ▼
┌─────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│  TimerEntry     │     │TimerEventHandler │     │  EventHandler    │
│  (Timer Data)   │     │ (timerfd Wrapper)│     │  (Callback Base) │
└─────────────────┘     └──────────────────┘     └──────────────────┘
```

### 2.2 Timer Coalescing (Interval Sharing)

Multiple timers with the **same interval** share a single Linux timerfd to reduce kernel resource usage:

```cpp
// Timer groups entries by interval
std::map<uint32_t, TimerEntryList> intervalToTimers_;

// Example: 3 timers at 100ms share one timerfd
// intervalToTimers_[100] = [entry1, entry2, entry3]
// All trigger together when the 100ms timerfd fires
```

### 2.3 Data Structures

```cpp
// Timer Entry - stores user callback and metadata
struct TimerEntry {
    uint32_t       timerId;    // Unique ID (returned to user)
    uint32_t       interval;   // Milliseconds
    TimerCallback  callback;   // User-provided function
    bool           once;       // One-shot vs repeating
    int            timerFd;    // Associated timerfd handle
};

// Mappings
std::map<uint32_t, TimerEntryList> intervalToTimers_;  // interval -> entries
std::map<uint32_t, TimerEntryPtr> timerToEntries_;     // timerId -> entry
std::map<uint32_t, uint32_t> timers_;                  // timerFd -> interval
```

### 2.4 Design Decisions

| Decision | Rationale |
|----------|-----------|
| **timerfd + epoll** | Kernel-level precision; avoids busy-waiting |
| **Separate thread per Timer** | Non-blocking; user thread not affected |
| **Interval coalescing** | Reduces kernel timer objects (N timers → M intervals) |
| **Recursive mutex** | Allows nested lock acquisition in callbacks |
| **Shared_ptr for handlers** | Safe lifecycle management during callbacks |

---

## 3. Flow: Timer Registration and Callback

### 3.1 Setup Flow

```
Timer::Setup()
    │
    ├──▶ EventReactor::SwitchOn()     // Set switch_ = true
    │
    ├──▶ Spawn Thread ──▶ Timer::MainLoop()
                              │
                              ├──▶ prctl(PR_SET_NAME)     // Set thread name
                              ├──▶ EventReactor::SetUp()
                              │         └──▶ epoll_create1()
                              │
                              └──▶ EventReactor::RunLoop()
                                        └──▶ while (switch_) { epoll_wait(); }
```

### 3.2 Timer Registration Flow

```
Timer::Register(callback, interval, once)
    │
    ├──▶ lock(mutex_)
    │
    ├──▶ Check existing interval
    │     └──▶ Timer::GetTimerFd(interval)
    │           └──▶ Return existing timerFd if interval already registered
    │
    ├──▶ If no existing timerFd:
    │     └──▶ Timer::DoRegister()
    │           └──▶ EventReactor::ScheduleTimer()
    │                 ├──▶ new TimerEventHandler(this, interval, once)
    │                 ├──▶ handler->SetTimerCallback(cb)
    │                 └──▶ handler->Initialize()
    │                       ├──▶ timerfd_create(CLOCK_MONOTONIC, ...)
    │                       ├──▶ clock_gettime()           // Get current time
    │                       ├──▶ Calculate next timeout (now + interval)
    │                       └──▶ timerfd_settime()         // Arm the timer
    │
    ├──▶ Generate unique timerId
    │
    ├──▶ Create TimerEntry and add to maps
    │     ├──▶ intervalToTimers_[interval].push_back(entry)
    │     └──▶ timerToEntries_[timerId] = entry
    │
    └──▶ return timerId
```

### 3.3 Timeout Callback Flow

```
epoll_wait() returns (timer fired)
    │
    ├──▶ EventDemultiplexer::Polling()
    │     └──▶ For each event:
    │           ├──▶ Find EventHandler by fd
    │           └──▶ Queue handler for dispatch
    │
    └──▶ Dispatch queued handlers
          │
          └──▶ EventHandler::HandleEvents(READ_EVENT)
                └──▶ readCallback_()  // TimerEventHandler::TimeOut()
                      │
                      ├──▶ read(timerFd, &expirations, ...)  // Clear timer
                      │
                      └──▶ callback_(timerFd)  // User callback via lambda
                            │
                            └──▶ Timer::OnTimer(timerFd)
                                  ├──▶ lock(mutex_)
                                  ├──▶ Find interval from timerFd
                                  ├──▶ Get all entries for this interval
                                  └──▶ For each entry:
                                        ├──▶ if (reactor ready) entry->callback()
                                        └──▶ if (once) mark for cleanup
```

### 3.4 Unregister Flow

```
Timer::Unregister(timerId)
    │
    ├──▶ lock(mutex_)
    │
    ├──▶ Find entry in timerToEntries_
    │
    ├──▶ Remove from intervalToTimers_[interval]
    │
    ├──▶ If one-shot timer:
    │     └──▶ EventReactor::CancelTimer(timerFd)
    │
    ├──▶ If interval now empty:
    │     ├──▶ intervalToTimers_.erase(interval)
    │     └──▶ DoUnregister(interval)  // Cancel shared timerFd
    │
    └──▶ timerToEntries_.erase(timerId)
```

### 3.5 Shutdown Flow

```
Timer::Shutdown(useJoin)
    │
    ├──▶ EventReactor::SwitchOff()    // switch_ = false
    │
    ├──▶ If timeoutMs_ == -1 and no events:
    │     └──▶ Schedule pseudo-task to unblock epoll_wait
    │
    ├──▶ If useJoin == true:
    │     └──▶ thread_.join()         // Wait for thread completion
    │     Else:
    │     └──▶ thread_.detach()       // Detach (not recommended)
    │
    └──▶ EventReactor::CleanUp()
          └──▶ Uninitialize all TimerEventHandlers
```

---

## 4. Integration: Dependencies and Usage

### 4.1 Build Dependencies (CMakeLists.txt)

```cmake
add_library(timer STATIC
    timer.cpp
    timer_event_handler.cpp
    event_demultiplexer.cpp
    event_handler.cpp
    event_reactor.cpp
)

target_include_directories(timer PUBLIC .)
auto_target_link_libraries(timer PRIVATE misc)
```

### 4.2 External Dependencies

| Dependency | Purpose | Header |
|------------|---------|--------|
| **misc** | Logging utilities (`utils_log.h`) | `utils_log.h` |
| **errors** | Error code framework | `errors.h`, `common_errors.h` |
| **sys/epoll.h** | Linux epoll I/O multiplexing | System |
| **sys/timerfd.h** | Linux timer file descriptors | System |
| **sys/prctl.h** | Thread naming | System |

### 4.3 Usage Example

```cpp
#include "timer.h"
#include <iostream>

// Create timer with name and timeout
OHOS::Utils::Timer timer("MyTimer", 1000);

// Setup the timer (starts background thread)
uint32_t ret = timer.Setup();
if (ret != OHOS::Utils::TIMER_ERR_OK) {
    // handle error
}

// Register a one-shot timer (500ms)
uint32_t timerId1 = timer.Register([]() {
    std::cout << "One-shot timer fired!" << std::endl;
}, 500, true);

// Register a repeating timer (1000ms interval)
uint32_t timerId2 = timer.Register([]() {
    std::cout << "Repeating timer fired!" << std::endl;
}, 1000, false);

// Later: unregister timers
timer.Unregister(timerId1);
timer.Unregister(timerId2);

// Shutdown (blocking join)
timer.Shutdown(true);
```

### 4.4 Error Codes

| Error Code | Value | Meaning |
|------------|-------|---------|
| `TIMER_ERR_OK` | 0 | Success |
| `TIMER_ERR_DEAL_FAILED` | Offset + EAGAIN | Operation failed |
| `TIMER_ERR_BADF` | Offset + EBADF | Bad file descriptor |
| `TIMER_ERR_INVALID_VALUE` | Offset + EINVAL | Invalid parameter |

### 4.5 Thread Safety Notes

1. **Setup/Shutdown** must be called from the same thread or externally synchronized
2. **Register/Unregister** are thread-safe (protected by mutex)
3. **Callbacks** execute on the timer's internal thread
4. **Detaching** the thread (useJoin=false) is not recommended due to potential use-after-free

### 4.6 Performance Considerations

- **Timer Precision**: ~100μs response time for timeout events
- **Scalability**: Interval coalescing reduces kernel timer objects
- **Memory**: Each timer entry uses ~64 bytes + callback overhead
- **CPU**: Configurable epoll timeout; -1 for indefinite wait (lowest CPU usage)
