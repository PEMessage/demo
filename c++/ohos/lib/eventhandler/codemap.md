# OpenHarmony EventHandler Library - Codemap

## Responsibility

The EventHandler library provides a **message/event handling framework** for OpenHarmony applications. It implements:

- **Event Loop**: Thread-based event processing mechanism similar to Android's Looper/Handler
- **Message Queue**: Priority-based event scheduling with multiple priority levels
- **Task Scheduling**: Delayed, timed, and periodic task execution
- **File Descriptor Monitoring**: Epoll-based I/O event handling
- **Thread Management**: Automatic thread creation, lifecycle management, and cleanup
- **FFRT Integration**: Optional Fiber Task Runtime support for high-performance scenarios

## Design

### Core Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         EventHandler                            │
│  - High-level API for posting events/tasks                     │
│  - Inherits from std::enable_shared_from_this                  │
│  - ProcessEvent() virtual method for customization             │
└───────────────────────┬─────────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────────┐
│                        EventRunner                              │
│  - Thread management and lifecycle                             │
│  - Event loop execution (Run/Stop)                             │
│  - Thread modes: NEW_THREAD, FFRT, NO_WAIT                     │
└───────────────────────┬─────────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────────┐
│                     EventInnerRunner                            │
│  - Internal runner implementation                              │
│  - Thread-local current runner storage                         │
└───────────────────────┬─────────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────────┐
│                        EventQueue                               │
│  - Abstract base for queue implementations                     │
│  - Priority levels: VIP, IMMEDIATE, HIGH, LOW, IDLE            │
└───────────┬───────────────────────────┬─────────────────────────┘
            │                           │
┌───────────▼───────────┐   ┌───────────▼───────────┐
│    EventQueueBase     │   │    EventQueueFFRT     │
│  - Standard queue     │   │  - FFRT-based queue   │
│  - Epoll I/O waiter   │   │  - Fiber runtime      │
└───────────┬───────────┘   └───────────────────────┘
            │
┌───────────▼─────────────────────────────────────────────────────┐
│                        IoWaiter                                 │
│  - EpollIoWaiter: epoll-based I/O multiplexing                 │
│  - DeamonIoWaiter: daemon thread I/O waiter                    │
│  - NoneIoWaiter: no-op waiter (fallback)                       │
└─────────────────────────────────────────────────────────────────┘
```

### Event Priority Levels

| Priority | Value | Description |
|----------|-------|-------------|
| VIP | 0 | Highest priority, distributed until completed |
| IMMEDIATE | 1 | Distributed at once if possible |
| HIGH | 2 | Sorted by handle time, before LOW |
| LOW | 3 | Normal priority, sorted by handle time |
| IDLE | 4 | Only distributed when no other events |

### Thread Modes

- **NEW_THREAD**: Creates dedicated thread for event loop
- **FFRT**: Uses Fiber Task Runtime for high-performance task scheduling
- **NO_WAIT**: Non-blocking event processing mode

## Flow

### Event Posting Flow

```
┌─────────────────┐
│   Application   │
└────────┬────────┘
         │ PostTask/SendEvent
         ▼
┌─────────────────┐     ┌─────────────────┐
│  EventHandler   │────▶│  Create Task    │
│                 │     │  (InnerEvent)   │
└────────┬────────┘     └─────────────────┘
         │
         │ eventRunner_->GetEventQueue()->Insert()
         ▼
┌─────────────────┐     ┌─────────────────┐
│   EventQueue    │────▶│  Sort by time   │
│   (Insert)      │     │  & priority     │
└────────┬────────┘     └─────────────────┘
         │
         │ Notify IoWaiter
         ▼
┌─────────────────┐
│   IoWaiter      │
│  (Wake thread)  │
└─────────────────┘
```

### Event Processing Flow

```
┌─────────────────┐
│  EventRunner    │
│  (ThreadMain)   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│   EventQueue    │────▶│  GetEvent()     │
│   (GetEvent)    │     │  (Block if empty)│
└────────┬────────┘     └─────────────────┘
         │
         │ Return event
         ▼
┌─────────────────┐     ┌─────────────────┐
│EventInnerRunner │────▶│ExecuteEventHandler│
│    (Run)        │     │                 │
└────────┬────────┘     └─────────────────┘
         │
         │ handler->DistributeEvent()
         ▼
┌─────────────────┐     ┌─────────────────┐
│  EventHandler   │────▶│ ProcessEvent()  │
│(DistributeEvent)│     │  (virtual)      │
└─────────────────┘     └─────────────────┘
```

### File Descriptor Event Flow

```
┌─────────────────┐
│   epoll_wait    │
└────────┬────────┘
         │ FD event
         ▼
┌─────────────────┐
│ HandleFileDescriptorEvent
└────────┬────────┘
         │ Post task to handler
         ▼
┌─────────────────┐     ┌─────────────────┐
│  FileDescriptor │────▶│ OnReadable()    │
│    Listener     │     │ OnWritable()    │
│                 │     │ OnShutdown()    │
│                 │     │ OnException()   │
└─────────────────┘     └─────────────────┘
```

### Sync Event Flow

```
┌─────────────────┐
│ SendSyncEvent   │
└────────┬────────┘
         │
         ├─────────────────────────────┐
         │ Same runner?                │ Different runner
         ▼                             ▼
┌─────────────────┐           ┌─────────────────┐
│ DistributeEvent │           │ Create Waiter   │
│   (direct)      │           │ SendEvent()     │
└─────────────────┘           │ Wait() <───────┼───────┐
                               └─────────────────┘       │
                                                         │
                               ┌─────────────────┐       │
                               │ DistributeEvent │───────┘
                               │ Notify waiter   │
                               └─────────────────┘
```

## Key Classes

### EventHandler (`interfaces/inner_api/event_handler.h`)
- Main API for posting events and tasks
- Virtual `ProcessEvent()` for event handling
- Thread-local current handler tracking

### EventRunner (`interfaces/inner_api/event_runner.h`)
- Factory for creating event runners
- Thread management and lifecycle
- Main event runner support (`GetMainEventRunner()`)

### EventQueue (`interfaces/inner_api/event_queue.h`)
- Abstract queue interface
- Priority-based event management
- File descriptor listener support

### EventQueueBase (`frameworks/eventhandler/include/event_queue_base.h`)
- Standard queue implementation
- Sub-queue array for priorities (VIP, IMMEDIATE, HIGH, LOW)
- Idle event list for IDLE priority
- History event tracking (32 events)

### InnerEvent (`interfaces/inner_api/inner_event.h`)
- Event data structure
- Supports: event ID, callback task, smart pointers
- Waiter for sync events
- HiTrace integration

### FileDescriptorListener (`interfaces/inner_api/file_descriptor_listener.h`)
- Callback interface for FD events
- Types: LTYPE_VSYNC, LTYPE_UV, LTYPE_MMI, LTYPE_WEBVIEW

## File Structure

```
lib/eventhandler/
├── CMakeLists.txt                 # Build configuration
├── interfaces/inner_api/          # Public API headers
│   ├── event_handler.h            # Main EventHandler class
│   ├── event_runner.h             # EventRunner class
│   ├── event_queue.h              # EventQueue abstract class
│   ├── inner_event.h              # InnerEvent class
│   ├── file_descriptor_listener.h # FD listener interface
│   ├── event_handler_errors.h     # Error codes
│   ├── dumper.h                   # Debug dumping
│   └── ...
└── frameworks/eventhandler/
    ├── include/                   # Internal headers
    │   ├── event_queue_base.h     # Standard queue impl
    │   ├── event_inner_runner.h   # Internal runner
    │   ├── epoll_io_waiter.h      # Epoll waiter
    │   └── ...
    └── src/                       # Implementation files
        ├── event_handler.cpp      # EventHandler impl
        ├── event_runner.cpp       # EventRunner impl
        ├── event_queue_base.cpp   # EventQueueBase impl
        ├── event_queue.cpp        # EventQueue base impl
        ├── inner_event.cpp        # InnerEvent impl
        └── ...
```

## Integration

### Dependencies

From `CMakeLists.txt`:
- **libsec**: Security library
- **nocopyable**: Non-copyable/movable base classes
- **misc**: Miscellaneous utilities
- **hilog**: Logging framework
- **hitrace**: Performance tracing
- **syspara**: System parameters
- **ffrt**: Fiber Task Runtime (optional)

### Build Targets

```cmake
add_library(eventhandler STATIC
    frameworks/eventhandler/src/file_descriptor_listener.cpp
    frameworks/eventhandler/src/none_io_waiter.cpp
    frameworks/eventhandler/src/inner_event.cpp
    frameworks/eventhandler/src/event_handler.cpp
    frameworks/eventhandler/src/event_queue.cpp
    frameworks/eventhandler/src/event_queue_ffrt.cpp
    frameworks/eventhandler/src/deamon_io_waiter.cpp
    frameworks/eventhandler/src/ffrt_descriptor_listener.cpp
    frameworks/eventhandler/src/event_runner.cpp
    frameworks/eventhandler/src/event_queue_base.cpp
    frameworks/eventhandler/src/native_implement_eventhandler.cpp
    frameworks/eventhandler/src/epoll_io_waiter.cpp
)
```

### Usage Pattern

```cpp
// Create runner (starts new thread)
auto runner = EventRunner::Create("MyThread");

// Create handler attached to runner
auto handler = std::make_shared<EventHandler>(runner);

// Post a task
handler->PostTask([]() {
    // Task executed on runner's thread
}, "MyTask", delayMs, EventQueue::Priority::HIGH);

// Send an event
handler->SendEvent(eventId, param, delayMs);

// Process events (override in subclass)
void ProcessEvent(const InnerEvent::Pointer &event) override {
    // Handle event
}
```

## Special Features

### Barrier Mode
- Synchronization mechanism for VSYNC tasks
- Blocks lower priority events until barrier is cleared

### Observer Pattern
- `EventRunnerObserver` for lifecycle notifications
- Stages: BEFORE_WAITING, AFTER_WAITING, VIP_EXISTED, VIP_NONE
- Used by ArkTS GC integration

### Timeout Monitoring
- Delivery timeout: Time from send to start processing
- Distribute timeout: Time spent processing event
- HiChecker integration for slow event detection

### FFRT Support
- Optional fiber-based task runtime
- `EventQueueFFRT` for fiber-aware queue
- `ffrt_this_task_get_id()` for fiber detection

### File Descriptor Monitoring
- Epoll-based I/O multiplexing
- Support for readable/writable/shutdown/exception events
- Daemon I/O waiter for system-wide monitoring
