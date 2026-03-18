# Thread Extensions Library (thread_ex)

## Overview

The Thread Extensions library provides a C++ wrapper around POSIX threads (pthread) for OpenHarmony, offering a higher-level abstraction for thread management with lifecycle control, priority configuration, and graceful shutdown mechanisms.

---

## Responsibility

### Thread Utilities and Extensions

The library is responsible for:

1. **Thread Lifecycle Management**
   - Thread creation and initialization
   - Thread naming and priority configuration
   - Graceful thread termination (sync/async)

2. **Thread Control Primitives**
   - `Start()`: Creates and starts a child thread with configurable name, priority, and stack size
   - `NotifyExitSync()`: Synchronously signals thread exit, blocking until thread terminates
   - `NotifyExitAsync()`: Asynchronously signals thread exit without blocking

3. **Thread State Monitoring**
   - `IsRunning()`: Check if thread is currently active
   - `IsExitPending()`: Check if exit has been requested
   - `ReadyToWork()`: Hook for pre-execution readiness check

4. **Priority Management**
   - `THREAD_PROI_NORMAL` (0): Normal priority
   - `THREAD_PROI_LOW` (10): Low priority
   - `THREAD_PROI_LOWEST` (19): Lowest priority

---

## Design

### Thread Management Patterns

#### 1. Template Method Pattern

The `Thread` class uses the **Template Method** pattern:

```cpp
class Thread {
public:
    ThreadStatus Start(...);          // Fixed: Thread creation/setup
    virtual bool Run() = 0;           // Variable: Override in derived class
    virtual void NotifyExitAsync();   // Variable: Can override
    virtual bool ReadyToWork();       // Variable: Can override
};
```

**Key Design Decision**: The `Run()` method is pure virtual, forcing derived classes to implement their own thread logic while the base class handles all infrastructure.

#### 2. RAII & Detached Threads

Threads are created in **detached mode** (`PTHREAD_CREATE_DETACHED`):

- No explicit join required
- Thread resources auto-released on exit
- ThreadParam struct manages initialization data via heap allocation

#### 3. State Machine Pattern

Thread lifecycle states managed via flags:

```
[Idle] --Start()--> [Running] --NotifyExit*--> [Exiting] --> [Terminated]
                              --Run() returns false--> [Terminated]
```

**State Flags**:
- `running_`: Thread is actively executing
- `exitPending_`: Exit has been requested
- `status_`: Last operation result

#### 4. Proxy Pattern for Thread Initialization

The `ThreadParam::Proxy()` static method acts as a trampoline:

```cpp
pthread_create() -> ThreadParam::Proxy() -> setpriority() + prctl() -> ThreadStart() -> Run()
```

This allows setting thread attributes (name, priority) before user code executes.

#### 5. Condition Variable for Synchronization

- `cvThreadExited_`: Signals when thread actually terminates
- Used by `NotifyExitSync()` for blocking wait
- Protected by `lock_` mutex

---

## Flow

### Thread Creation and Management

#### 1. Thread Creation Flow

```
Thread::Start(name, priority, stack)
    ├── Lock acquired (thread safety)
    ├── Validate: Check if already running
    ├── Initialize state: running_=true, exitPending_=false
    ├── Create ThreadParam struct
    │       ├── startRoutine = ThreadStart
    │       ├── args = this (Thread instance)
    │       ├── name = user-provided name
    │       └── priority = user-provided priority
    ├── CreatePThread(para, stack, &thread_)
    │       ├── pthread_attr_init()
    │       ├── pthread_attr_setdetachstate(DETACHED)
    │       ├── pthread_create() with ThreadParam::Proxy
    │       └── pthread_attr_destroy()
    └── Return OK or error
```

#### 2. Thread Execution Flow

```
ThreadParam::Proxy(ThreadParam* t)
    ├── Extract: startRoutine, args, priority, name
    ├── Delete ThreadParam (cleanup)
    ├── setpriority(PRIO_PROCESS, 0, prio)
    ├── prctl(PR_SET_NAME, name)  [Linux-specific]
    └── Call startRoutine(args) → ThreadStart(this)

ThreadStart(void* args)
    ├── self = static_cast<Thread*>(args)
    ├── do-while loop:
    │       ├── if first iteration: ReadyToWork() check
    │       ├── Run() [pure virtual - user implementation]
    │       ├── Check result and exitPending_
    │       └── if exit needed:
    │               ├── running_ = false
    │               ├── thread_ = INVALID_PTHREAD_T
    │               ├── cvThreadExited_.notify_all()
    │               └── break
    └── return 0
```

#### 3. Thread Exit Flows

**Synchronous Exit (NotifyExitSync)**:
```
Caller Thread                      Target Thread
     │                                  │
     ├── NotifyExitSync() ─────────────>│
     │    ├── Validate: not self        │
     │    ├── lock_                     │
     │    ├── exitPending_ = true       │
     │    ├── while(running_)           │
     │    │      cvThreadExited_.wait() │
     │    │      <---notification------─┤ThreadStart exit
     │    └── return status_            │
```

**Asynchronous Exit (NotifyExitAsync)**:
```
Caller Thread                      Target Thread
     │                                  │
     ├── NotifyExitAsync() ────────────>│
     │    ├── lock_                     │
     │    └── exitPending_ = true       │
     │(returns immediately)              │
     │                                  ├── Run() checks IsExitPending()
     │                                  └── exits on next iteration
```

#### 4. Thread Loop Behavior

The `Run()` method is called in a loop until:
- `Run()` returns `false` (user signals completion)
- `NotifyExitAsync()` or `NotifyExitSync()` is called
- `ReadyToWork()` returns `false` on first iteration

**First-Iteration Special Handling**:
```cpp
if (first) {
    if (ReadyToWork() && !IsExitPending()) {
        result = Run();
    }
} else {
    result = Run();  // No ReadyToWork check
}
```

---

## Integration

### Dependencies

#### Build System (CMakeLists.txt)

```cmake
add_library(thread_ex STATIC thread_ex.cpp)
target_include_directories(thread_ex PUBLIC .)
auto_target_link_libraries(thread_ex PRIVATE misc)
```

**Key Dependencies**:
- `misc` library: Provides `UTILS_LOGD` logging macro
- pthread (system library)

#### Header Dependencies

```cpp
// Standard Headers
#include <pthread.h>           // POSIX threading
#include <string>              // Thread naming
#include <mutex>               // Synchronization
#include <condition_variable>  // Exit notification

// System Headers
#include <sys/resource.h>      // setpriority()
#include <sys/prctl.h>         // prctl() for thread naming

// Internal Headers
#include "utils_log.h"         // Logging utilities
```

### Usage Patterns

#### 1. Basic Thread Implementation

```cpp
#include "thread_ex.h"

class WorkerThread : public OHOS::Thread {
protected:
    bool Run() override {
        while (!IsExitPending()) {
            // Do work...
        }
        return true;  // Continue running (false = stop)
    }
};

// Usage
WorkerThread worker;
worker.Start("WorkerThread", OHOS::THREAD_PROI_NORMAL, 0);

// Later...
worker.NotifyExitSync();  // Block until thread exits
```

#### 2. Thread with Custom Readiness

```cpp
class ConditionalThread : public OHOS::Thread {
protected:
    bool ReadyToWork() override {
        return resourceInitialized_;  // Check preconditions
    }

    bool Run() override {
        // Main work loop
        return true;
    }

private:
    bool resourceInitialized_ = true;
};
```

#### 3. Asynchronous Shutdown

```cpp
class ServiceThread : public OHOS::Thread {
protected:
    bool Run() override {
        while (!IsExitPending()) {
            ProcessRequests();
        }
        Cleanup();  // Graceful cleanup
        return false;  // Stop loop
    }
};

// Usage
ServiceThread service;
service.Start("Service", OHOS::THREAD_PROI_LOW, 0);

// Signal exit (non-blocking)
service.NotifyExitAsync();
// Continue with other work...
```

### Error Handling

| Status Code | Meaning | Typical Cause |
|-------------|---------|---------------|
| `OK` | Success | Operation completed successfully |
| `INVALID_OPERATION` | Invalid state | Thread already running |
| `UNKNOWN_ERROR` | System error | pthread_create failed |
| `WOULD_BLOCK` | Deadlock risk | Calling NotifyExitSync from self |

### Thread Safety

- **Thread-safe**: `Start()`, `NotifyExitSync()`, `NotifyExitAsync()`, `IsRunning()`, `IsExitPending()`
- **Not thread-safe**: `Run()` (called only by thread itself), `ReadyToWork()`
- **Synchronization**: All public methods use `lock_` mutex

### Platform Considerations

- **Linux-specific**: Uses `prctl(PR_SET_NAME)` for thread naming
- **POSIX**: Relies on pthread library
- **OpenHarmony**: Part of c_utils library, Apache 2.0 licensed

### Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `INVALID_PTHREAD_T` | -1 | Sentinel for invalid thread ID |
| `MAX_THREAD_NAME_LEN` | 15 | Linux thread name limit |

---

## File Structure

```
lib/thread_ex/
├── CMakeLists.txt      # Build configuration
├── thread_ex.h         # Public API (Thread class, enums)
├── thread_ex.cpp       # Implementation
└── codemap.md          # This documentation
```

## Key Classes/Structs

| Name | Type | Purpose |
|------|------|---------|
| `Thread` | Class | Main thread abstraction |
| `ThreadStatus` | Enum | Operation result codes |
| `ThreadPrio` | Enum | Priority levels |
| `ThreadParam` | Struct | Initialization parameters proxy |
| `CreatePThread` | Function | Thread creation helper |

## Thread Lifecycle Diagram

```
┌─────────────────┐
│  Thread Created │
│   (Constructor) │
└────────┬────────┘
         │
         v
┌─────────────────┐     Start() failed     ┌─────────────┐
│      Idle       │───────────────────────>│    Error    │
│  (running_=false)│                       └─────────────┘
└────────┬────────┘
         │ Start() success
         v
┌─────────────────┐
│    Running      │<────────────────────────┐
│ (running_=true) │                         │
└────────┬────────┘                         │
         │                                 │
    ┌────┴────┐                            │
    │         │                            │
    v         v                            │
┌────────┐ ┌──────────┐                    │
│Run()   │ │NotifyExit│                    │
│returns │ │*() called│                    │
│false   │ │          │                    │
└────┬───┘ └────┬─────┘                    │
     │          │                          │
     └────┬─────┘                          │
          │                                │
          v                                │
┌─────────────────┐                        │
│    Exiting      │                        │
│(exitPending_    │                        │
│    =true)       │                        │
└────────┬────────┘                        │
         │                                │
         v                                │
┌─────────────────┐   ThreadStart loop    │
│   Terminated    │────────────────────────┘
│ (running_=false)│   (restarts if needed)
└─────────────────┘
         │
         v
┌─────────────────┐
│ Thread Destroyed│
│  (Destructor)   │
└─────────────────┘
```
