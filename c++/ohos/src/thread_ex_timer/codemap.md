# thread_ex_timer - Thread-Based Timer Daemon Demo

## Responsibility

Advanced demonstration of combining Thread with timer functionality. Implements a complete timer daemon that runs in its own thread, supports multiple timers (repeating and one-shot), and uses message passing for thread-safe API.

## Design

### TimerDaemonThread Architecture
```cpp
class TimerDaemonThread : public Thread {
    // Message queue for thread-safe API
    queue<TimerMessage> msgQueue;
    
    // Active timers storage
    map<int, TimerInfo> timers;
    
    // Thread-safe synchronization
    mutex queueMutex, timerMutex;
    condition_variable queueCV;
    
    // Next timer ID generator
    atomic<int> nextTimerId;
};
```

### Message Passing API
```cpp
enum class TimerMessageType {
    CREATE_TIMER,
    STOP_TIMER,
    EXIT_DAEMON
};

struct TimerMessage {
    TimerMessageType type;
    int timerId;
    int intervalMs;
    TimerCallback callback;
    bool repeat;
};

// Public API (thread-safe)
int CreateTimer(int intervalMs, TimerCallback callback, bool repeat);
void StopTimer(int timerId);
void Shutdown();
```

### TimerInfo Structure
```cpp
struct TimerInfo {
    int timerId;
    int intervalMs;
    TimerCallback callback;
    bool repeat;
    chrono::steady_clock::time_point nextTrigger;
};
```

## Flow

```
main.cpp
├── Create TimerDaemonThread
├── Start the daemon thread
│   └── ReadyToWork() → "TimerDaemonThread is ready!"
│
├── CreateTimer(1000, callback1, repeat=true)   // 1s repeating
├── CreateTimer(3000, callback2, repeat=false)  // 3s one-shot
├── CreateTimer(500, callback3, repeat=true)    // 500ms fast
│   └── Messages sent to queue, processed by daemon
│
├── Sleep 5 seconds (timers firing)
│   └── Daemon thread processes message queue
│   └── Checks timer triggers
│   └── Executes callbacks
│
├── StopTimer(timer3)  // Stop fast timer
├── Sleep 3 more seconds
├── CreateTimer(2000, callback4, false)  // Final one-shot
├── CreateTimer(1500, lambda, true)      // Capturing lambda
│   └── Demonstrates variable capture in callbacks
│
├── Shutdown()
│   └── EXIT_DAEMON message sent
│   └── Daemon exits Run() loop
│
└── Wait for daemon to stop
```

## Integration

### Dependencies
- **thread_ex**: Thread abstraction
- **misc**: Utilities

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE thread_ex misc)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
=== Timer Daemon Thread Demo ===
TimerDaemonThread is ready to work!

1. Creating repeating timer (every 1 second)
Timer 1 created: interval=1000ms, repeat=1
2. Creating one-shot timer (after 3 seconds)
Timer 2 created: interval=3000ms, repeat=0
3. Creating fast repeating timer (every 500ms)
Timer 3 created: interval=500ms, repeat=1

Letting timers run for 5 seconds...
Triggering timer 1
Repeating timer triggered! Count: 1
Triggering timer 3
Fast timer: 1
...
```

### Key APIs

| API | Purpose |
|-----|---------|
| `CreateTimer(intervalMs, callback, repeat)` | Create new timer |
| `StopTimer(timerId)` | Stop and remove timer |
| `Shutdown()` | Stop daemon thread |
| `GetActiveTimerCount()` | Query active timers |
| `NotifyExitAsync()` | Async shutdown request |

### Timer Types

**Repeating Timer:**
```cpp
int timer = timerDaemon.CreateTimer(1000, []() {
    // Called every 1000ms
}, true);
```

**One-Shot Timer:**
```cpp
int timer = timerDaemon.CreateTimer(3000, []() {
    // Called once after 3000ms
}, false);
```

**Lambda with Capture:**
```cpp
string name = "TimerUser";
int counter = 0;
int timer = timerDaemon.CreateTimer(1500, [&name, &counter]() {
    cout << "Hello " << name << "! Counter: " << ++counter << endl;
}, true);
```

## Key Implementation Details

### Thread Safety
- All public APIs use message queue for thread-safe operation
- Callbacks execute on daemon thread (not caller thread)
- Timer storage protected by mutex

### Message Processing
```cpp
void ProcessMessageQueue() {
    unique_lock<mutex> lock(queueMutex);
    queueCV.wait_for(lock, chrono::milliseconds(100));
    // Process all pending messages
}
```

### Timer Triggering
```cpp
void ProcessTimers() {
    auto now = chrono::steady_clock::now();
    for (auto& pair : timers) {
        if (now >= pair.second.nextTrigger) {
            TriggerCallback(pair.first);
            UpdateNextTrigger(pair.second);
        }
    }
}
```

## Notes

- Demonstrates advanced thread programming patterns
- Message queue decouples API from implementation
- Condition variable with timeout allows periodic timer checks
- One-shot timers automatically cleaned up after triggering
- Callback exceptions caught and logged (don't crash daemon)
