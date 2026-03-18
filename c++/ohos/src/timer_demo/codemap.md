# timer_demo - Timer Framework Demo

## Responsibility

Demonstrates the OpenHarmony `Utils::Timer` class for scheduling periodic callbacks. Shows how to register multiple timers with different intervals, start the timer loop, and properly shut down the timer service.

## Design

### Timer Architecture
```cpp
Utils::Timer timer("timer_test");  // Named timer instance

// Callback functions (lambda)
Timer::TimerCallback func1 = [](){
    static int count = 0;
    cout << "Hi from func1, count " << count++ << endl;
};

// Register callbacks with intervals
uint32_t timer1Id = timer.Register(func1, 1000);  // Every 1 second
uint32_t timer2Id = timer.Register(func2, 2000);  // Every 2 seconds
```

### Lifecycle Management
```cpp
timer.Setup();        // Initialize and start timer thread
timer.Shutdown(true); // Stop and cleanup (true = wait for pending)
```

## Flow

```
main.cpp
├── Create Timer instance with name
├── Define callback functions (lambdas)
├── timer.Setup()
│   └── Start internal timer thread
├── Register callback1 (1000ms interval)
├── Register callback2 (2000ms interval)
├── Sleep for 300 seconds (demo duration)
├── timer.Shutdown(true)
│   └── Stop timer thread gracefully
└── Return 0
```

## Integration

### Dependencies
- **timer**: `/home/zhuojw/demo_new/c++/ohos/lib/timer/`

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE timer)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
Hi from func1, count 0
Hi from func2, count 0
Hi from func1, count 1
Hi from func1, count 2
Hi from func2, count 1
...
```

### Key APIs

| API | Purpose |
|-----|---------|
| `Timer(name)` | Create named timer instance |
| `Setup()` | Initialize and start timer thread |
| `Register(callback, intervalMs)` | Register periodic callback |
| `Shutdown(wait)` | Stop timer, wait for pending if true |
| `Unregister(timerId)` | Remove specific timer (not shown in demo) |

## Notes

- Timer callbacks execute on a dedicated timer thread
- Multiple timers can be registered with a single Timer instance
- Timer IDs are uint32_t and can be used to unregister specific timers
- The demo runs for 300 seconds to demonstrate continuous operation
- Always call `Shutdown()` before destroying the Timer to prevent crashes
