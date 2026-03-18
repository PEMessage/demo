# timer_node_demo - Timer-Driven Node Device Simulation

## Responsibility

Demonstrates a timer-based node device simulation with multiple operating modes (blink, const, duty). Shows how to manage multiple node handles with different configurations and how handles can preempt each other.

## Design

### Mode System
```cpp
// Three operating modes using std::variant
using SubMode = std::variant<BlinkMode, ConstMode, DutyMode>;

struct BlinkMode { int interval; };     // Toggle on/off
struct ConstMode {};                     // Constant on
struct DutyMode { int interval; int duty; };  // PWM-like duty cycle
```

### NodeDevice and NodeHandle
```cpp
class NodeDevice {
    // Manages the actual device state
    // Creates/deletes NodeHandle instances
    // Handles timer-based state updates
};

class NodeHandle {
    // Represents a client handle to the device
    // Has enable/disable state
    // Can be light() or dark()
    // Supports slot-based configuration (USER/SYSTEM)
};
```

### Configuration Slots
```cpp
// Handle has two configuration slots
enum Slot { USER, SYSTEM };

// Each slot can have different mode configurations
void setMode(Slot slot, const Config& config);
void switchSlot(Slot slot);
```

## Flow

```
main.cpp
├── testEnableDisable()
│   ├── Create NodeDevice (disabled)
│   ├── Create NodeHandle with BlinkMode
│   ├── Test disable/enable
│   └── Cleanup
│
├── testConfigSaveRestore()
│   ├── Create NodeDevice
│   ├── Create handle with BlinkMode
│   ├── Test dark/light operations
│   ├── Test slot switching
│   └── Cleanup
│
└── testNodePreemptive()
    ├── Create NodeDevice
    ├── Create handle1 (BlinkMode)
    ├── Create handle2 (ConstMode) - preempts handle1
    ├── Create handle3 (DutyMode) - preempts handle2
    ├── Delete handle2 - reverts to handle3
    ├── Delete handle3 - reverts to handle1
    └── Cleanup
```

## Integration

### Dependencies
- **timer**: Utils::Timer for scheduling
- **nocopyable**: DISALLOW_COPY_AND_MOVE macro
- **misc**: Miscellaneous utilities

### Files
- `main.cpp`: Demo scenarios
- `node_device.h/cpp`: Device management
- `node_handle.h/cpp`: Handle management
- `misc.h`: Shared types and utilities

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp node_handle.cpp node_device.cpp node_devices.cpp)
auto_target_link_libraries(app PRIVATE timer nocopyable)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
==== testEnableDisable
---- Now create device, enabled set to false, should not have [*]
---- Now use blink to init handle
---- Now disable it
---- Now reenable it, should back to blink
```

### Key APIs

| API | Purpose |
|-----|---------|
| `NodeDevice(timer, path, enabled)` | Create device |
| `createHandle(config)` | Create handle with config |
| `deleteHandle(handle)` | Remove handle |
| `handle.light()` | Turn on (highest precedence) |
| `handle.dark()` | Turn off |
| `handle.enable()` / `disable()` | Control handle state |
| `handle.setMode(slot, mode)` | Configure slot |
| `handle.switchSlot(slot)` | Switch active slot |

### Configuration

```cpp
Config{
    .enabled = true,
    .submode = BlinkMode{200}    // 200ms blink interval
}

Config{
    .enabled = true,
    .submode = ConstMode{}        // Always on
}

Config{
    .enabled = true,
    .submode = DutyMode{500, 70}  // 500ms period, 70% duty
}
```

## Notes

- Last-created active handle takes precedence
- When a handle is deleted, previous handle becomes active
- Slots allow pre-configured modes that can be switched instantly
- Uses recursive_mutex for thread safety
- Sanitizer flags enabled in build for memory safety checking
