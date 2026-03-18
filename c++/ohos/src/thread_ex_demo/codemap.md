# thread_ex_demo - Thread Abstraction Demo

## Responsibility

Demonstrates the OpenHarmony `Thread` class for creating and managing threads. Shows thread lifecycle (Start, Run, Stop), custom thread implementation by inheritance, and self-terminating thread pattern.

## Design

### Thread Inheritance Pattern
```cpp
class DemoThread : public Thread {
protected:
    // Called repeatedly while thread runs
    bool Run() override {
        static int counter = 0;
        std::cout << "DemoThread running, counter: " << counter++ << std::endl;
        sleep(1);
        return (counter < 5);  // Return false to stop
    }

public:
    // Called before Run() starts
    bool ReadyToWork() override {
        std::cout << "DemoThread is ready to work!" << std::endl;
        return true;
    }
};
```

### Lifecycle
```cpp
DemoThread thread;

// Start the thread
ThreadStatus status = thread.Start("DemoThread");

// Check if running
while (thread.IsRunning()) {
    sleep(1);
}

// Thread stops automatically when Run() returns false
```

## Flow

```
main.cpp
├── Demo 1: Self-terminating thread
│   ├── Create DemoThread instance
│   ├── thread.Start("DemoThread")
│   │   ├── ReadyToWork() called
│   │   └── Run() loop starts
│   ├── Wait while IsRunning()
│   │   └── Run() executes 5 times
│   ├── Run() returns false
│   └── Thread stops naturally
└── Demo completed
```

## Integration

### Dependencies
- **thread_ex**: Thread abstraction library

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE thread_ex)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
=== OHOS ThreadEx Demo ===

1. Self-terminating thread demo:
DemoThread is ready to work!
DemoThread running, counter: 0
DemoThread running, counter: 1
DemoThread running, counter: 2
DemoThread running, counter: 3
DemoThread running, counter: 4
DemoThread finished naturally

=== Demo completed ===
```

### Key APIs

| API | Purpose |
|-----|---------|
| `Thread` | Base class for threads |
| `Start(name)` | Start thread execution |
| `Run()` | Override - main thread work (return false to stop) |
| `ReadyToWork()` | Override - preparation before Run() |
| `IsRunning()` | Check if thread is active |
| `ThreadStatus::OK` | Success status |

### Thread Status Codes
```cpp
enum class ThreadStatus {
    OK = 0,
    INVALID_ARGUMENT,
    // ... other error codes
};
```

## Notes

- Thread terminates when `Run()` returns `false`
- `ReadyToWork()` is called once before the Run() loop
- Thread name is passed to `Start()` for debugging
- Thread object automatically cleans up on destruction
- For more complex scenarios, see `thread_ex_timer` demo
