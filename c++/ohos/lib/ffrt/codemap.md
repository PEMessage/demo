# lib/ffrt/ - Function Flow Runtime (FFRT)

## Responsibility

FFRT (Function Flow Runtime) is OpenHarmony's **asynchronous task scheduling and execution framework**. It provides:

- **Task-based concurrency**: Submit work as tasks rather than managing threads directly
- **Dependency-driven execution**: Tasks execute automatically when their dependencies are satisfied
- **QoS-aware scheduling**: Tasks are scheduled with different quality-of-service levels (background, utility, default, user_initiated)
- **Synchronization primitives**: FFRT-optimized mutex, condition variables, and queues
- **Event loop support**: Timer and epoll-based I/O event handling integrated with the task system

This directory contains **public API headers only** - the actual implementation resides in `libffrt.z.so` (distributed as a system library).

## Design

### Architecture Patterns

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │   Task   │  │  Queue   │  │  Timer   │  │   Loop   │   │
│  │  Submit  │  │  Submit  │  │  Start   │  │   Run    │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
└───────┼─────────────┼─────────────┼─────────────┼───────────┘
        │             │             │             │
        └─────────────┴─────────────┴─────────────┘
                          │
              ┌───────────▼───────────┐
              │     FFRT Runtime      │
              │    (libffrt.z.so)     │
              │  ┌─────────────────┐  │
              │  │  Task Scheduler │  │
              │  │  ├─ Dependency  │  │
              │  │  │   Tracking    │  │
              │  │  ├─ QoS Queues  │  │
              │  │  └─ Worker Pool │  │
              │  └─────────────────┘  │
              └───────────────────────┘
```

### Core Abstractions

1. **Tasks** (`task.h`)
   - Functions wrapped with execution context
   - Input/output dependencies (data or task handles)
   - Attributes: name, QoS, delay, priority, stack size, timeout

2. **Task Function Wrapper** (`ffrt_function_header_t`)
   ```c
   typedef struct {
       ffrt_function_t exec;     // Execute task
       ffrt_function_t destroy;  // Cleanup
       uint64_t reserve[2];
   } ffrt_function_header_t;
   ```

3. **QoS Levels** (`type_def.h`)
   - `ffrt_qos_inherit` (-1): Inherit from parent
   - `ffrt_qos_background` (0): Background tasks
   - `ffrt_qos_utility` (1): Utility/RT tasks
   - `ffrt_qos_default` (2): Default priority
   - `ffrt_qos_user_initiated` (3): User-initiated tasks

4. **Dependencies** (`ffrt_dependence_t`)
   - `ffrt_dependence_data`: Data dependency (void* pointer)
   - `ffrt_dependence_task`: Task dependency (task handle)

5. **Queues** (`queue.h`)
   - Serial queues: FIFO execution, single concurrent task
   - Concurrent queues: Multiple tasks execute in parallel with priority levels
   - Queue priorities: immediate, high, low, idle

6. **Synchronization** (`mutex.h`, `condition_variable.h`)
   - FFRT-aware mutex (cooperative scheduling)
   - Condition variables with timeout support
   - Sleep/yield operations that don't block worker threads

### API Layers

```
interfaces/
├── kits/
│   ├── ffrt.h           # Main include (auto-detects C/C++)
│   ├── c/
│   │   ├── task.h       # C task API
│   │   ├── queue.h      # C queue API
│   │   ├── mutex.h      # C mutex API
│   │   ├── condition_variable.h  # C condvar API
│   │   ├── timer.h      # C timer API
│   │   ├── loop.h       # C event loop API
│   │   ├── sleep.h      # C sleep API
│   │   └── type_def.h   # Common types
│   └── cpp/
│       ├── task.h       # C++ RAII wrappers (task_attr, task_handle, submit())
│       ├── queue.h      # C++ queue wrapper
│       ├── mutex.h      # C++ mutex wrapper
│       ├── condition_variable.h  # C++ condvar wrapper
│       └── sleep.h      # C++ sleep wrapper
└── inner_api/           # Internal/HDI APIs
    ├── ffrt_inner.h
    ├── c/init.h         # Subprocess initialization
    ├── c/task_ext.h     # Extended task APIs (skip, cgroup, escape)
    ├── c/thread.h       # Thread management
    └── cpp/future.h     # C++ future/promise integration
```

## Flow

### Task Submission Flow

```
1. Application calls submit()
   └─→ C++: create_function_wrapper(func)
       └─→ Allocates storage via ffrt_alloc_auto_managed_function_storage_base()
       └─→ Wraps in ffrt_function_header_t with exec/destroy callbacks

2. ffrt_submit_base(func, in_deps, out_deps, attr)
   └─→ Submits to FFRT runtime (libffrt.z.so)

3. Runtime schedules task:
   a. Check if all input dependencies satisfied
   b. Assign to QoS-level worker queue
   c. Wake worker thread or queue for execution

4. Task execution:
   └─→ Worker thread calls func->header.exec(func)
   └─→ User code runs
   └─→ On completion: notify dependent tasks

5. Cleanup:
   └─→ func->header.destroy(func) called
```

### Dependency Resolution

```
Task A ──┐
         ├─→ Task C (waits for A and B)
Task B ──┘

Data X ──┐
         ├─→ Task D (waits for X and Y to be produced)
Data Y ──┘
```

Tasks declare:
- **Input dependencies**: Tasks/data that must be ready before execution
- **Output dependencies**: Data this task will produce

The runtime builds a DAG and executes tasks in topological order.

### Queue Execution Flow

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ Serial Queue│     │ Concurrent  │     │ Main Queue  │
│ (FIFO)      │     │ Queue       │     │ (App UI)    │
├─────────────┤     ├─────────────┤     ├─────────────┤
│ Task 1      │     │ Task A      │     │ UI Task 1   │
│ Task 2      │     │ Task B      │     │ UI Task 2   │
│ Task 3      │     │ Task C      │     │ ...         │
└──────┬──────┘     │ (priority   │     └──────┬──────┘
       │            │  ordered)   │            │
       │            └──────┬──────┘            │
       │                   │                   │
       └───────────────────┼───────────────────┘
                           │
                    ┌──────▼──────┐
                    │ QoS Worker  │
                    │   Pool      │
                    └─────────────┘
```

### Event Loop Integration

```
ffrt_loop_create(queue)
    └─→ Creates epoll-based event loop tied to queue

ffrt_loop_epoll_ctl(loop, op, fd, events, data, cb)
    └─→ Register file descriptor with callback

ffrt_loop_timer_start(loop, timeout, data, cb, repeat)
    └─→ Schedule timer callback on loop

ffrt_loop_run(loop)
    └─→ Blocks, processing events and queue tasks
```

## Integration

### Build Dependencies

From `CMakeLists.txt`:
```cmake
target_include_directories(ffrt PUBLIC
    interfaces/kits       # Public API
    interfaces/inner_api  # Internal/HDI API
)

auto_target_link_libraries(ffrt PUBLIC
    libsec      # Security library
    nocopyable  # Utility
    misc        # Misc utilities
    hilog       # Logging
)
```

### Consumer Integration

**C++ Usage:**
```cpp
#include "ffrt.h"

// Simple task submission
ffrt::submit([]() {
    // Task code
}, ffrt::task_attr().name("my_task").qos(ffrt::qos_default));

// With dependencies
ffrt::task_handle h = ffrt::submit_h([](/* task */));
ffrt::submit([](/* uses result */), {h});  // Depends on h

// Wait for completion
ffrt::wait();
```

**C Usage:**
```c
#include "c/task.h"

ffrt_task_attr_t attr;
ffrt_task_attr_init(&attr);
ffrt_task_attr_set_name(&attr, "my_task");
ffrt_task_attr_set_qos(&attr, ffrt_qos_default);

ffrt_function_header_t* func = create_my_function();
ffrt_submit_base(func, NULL, NULL, &attr);
```

### System Integration Points

1. **HDI (Hardware Device Interface)**: Inner APIs used by system services
2. **Process Fork Support**: `ffrt_child_init()` for subprocess initialization
3. **Resource Management**: cgroup integration, CPU boost, worker escape mechanisms
4. **Logging**: Uses hilog for diagnostics

### Key Files Reference

| File | Purpose |
|------|---------|
| `interfaces/kits/ffrt.h` | Main public header |
| `interfaces/kits/c/task.h` | C task API (submit, wait, attributes) |
| `interfaces/kits/cpp/task.h` | C++ API with RAII wrappers |
| `interfaces/kits/c/queue.h` | Queue API (serial/concurrent) |
| `interfaces/kits/c/type_def.h` | Core types (QoS, dependencies, handles) |
| `interfaces/inner_api/c/init.h` | Subprocess initialization |
| `interfaces/inner_api/c/task_ext.h` | Extended/internal task APIs |
| `keep.c` | Empty placeholder (prevents empty library) |

### Notes

- This is an **interface-only** library - actual implementation is in system `libffrt.z.so`
- Threading model: Work-stealing thread pool per QoS level
- Cooperative scheduling: Tasks should use FFRT sync primitives (not pthread) to avoid blocking workers
- Supports both standalone tasks and queue-based task dispatch (similar to Grand Central Dispatch)
