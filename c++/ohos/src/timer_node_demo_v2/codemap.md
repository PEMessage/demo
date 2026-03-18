# timer_node_demo_v2 - Enhanced Timer Node with Manager Pattern

## Responsibility

Enhanced version of timer_node_demo with improved architecture. Introduces NodeManager for multi-device coordination, cleaner separation of concerns, and the Slot-based configuration system (USER/SYSTEM slots) moved to handle level.

## Design

### Namespace Organization
```cpp
namespace Node {
    class NodeDevice;    // Device with mode
    class NodeHandle;    // Handle with slot-based config
    class NodeHandles;   // Collection of handles for a device
    class NodeManager;   // Manages multiple devices
}
```

### NodeDevice (v2)
```cpp
class NodeDevice {
    // No longer manages handles directly
    // Focuses on: mode + timer + state
    // Uses std::variant for mode types
};

struct Mode {
    bool enabled;
    std::variant<BlinkMode, ConstMode, DutyMode> submode;
};
```

### NodeHandle with Slots (v2)
```cpp
class NodeHandle {
    // Two independent configuration slots
    static constexpr int USER = 0;
    static constexpr int SYSTEM = 1;
    
    void setMode(int slot, Mode mode);
    void switchSlot(int slot);
    void enable(int slot);
    void disable(int slot);
};
```

### NodeManager
```cpp
class NodeManager {
    // Manages multiple NodeDevice instances
    // Creates devices from InitOpts list
    // Provides iteration over devices/items
    
    std::initializer_list<InitOpts> devopts;
    // WARNING: Must not return initializer_list from function
};
```

## Flow

```
main.cpp
├── testDevice()
│   ├── Create Timer
│   ├── Create NodeDevice with InitOpts
│   ├── Set BlinkMode → update()
│   ├── Set DutyMode → update()
│   └── Cleanup
│
├── testHandles()
│   ├── Create Device (disabled)
│   ├── Create NodeHandles collection
│   ├── Create handle with BlinkMode
│   ├── Set USER slot to ConstMode
│   ├── Switch to USER slot
│   ├── Test enable/disable on different slots
│   └── Cleanup
│
└── testDevices()
    ├── Create NodeManager with 2 devices
    ├── Iterate and set ConstMode on all
    ├── Create handle group spanning devices
    ├── Selectively enable/disable handles
    └── Cleanup
```

## Integration

### Dependencies
- **timer**: Utils::Timer
- **nocopyable**: Copy prevention macros

### Files
- `main.cpp`: Test scenarios
- `node_device.h/cpp`: Device implementation
- `node_handle.h/cpp`: Handle implementation
- `node_handles.h/cpp`: Handle collection
- `node_manager.h/cpp`: Multi-device manager
- `misc.h`: Shared types

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp node_device.cpp node_handle.cpp node_handles.cpp node_manager.cpp)
auto_target_link_libraries(app PRIVATE timer nocopyable)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
==== testDevice
==== testHandles
---- Handle init with blink
---- Handle set user slot to Const
---- Handle switch Slot
==== testDevices
---- ConstMode on all devs
---- ConstMode off all devs
---- Create handles group
---- Only Keep MockName1, and disable another
```

### Key APIs

| API | Purpose |
|-----|---------|
| `NodeDevice(timer, InitOpts)` | Create device |
| `device.setMode(Mode)` | Set current mode |
| `device.update()` | Apply mode changes |
| `NodeHandles(device)` | Create handle collection |
| `handles.createHandle()` | Create new handle |
| `handle.setMode(slot, Mode)` | Configure slot |
| `handle.switchSlot(slot)` | Switch active slot |
| `NodeManager({InitOpts...})` | Create multi-device manager |
| `manager.allDevices()` | Iterate devices |
| `manager.allItems()` | Iterate device+handle pairs |

### InitOpts Pattern
```cpp
NodeDevice::InitOpts{
    .name = "DeviceName",
    .path = "/sys/path",
    .enabled = false  // Start disabled
}

NodeManager::InitOpts{
    .devopts = {"Name", "Path", false},
    .mode = Mode{true, BlinkMode{500}}
}
```

## Notes

- **CAUTION**: `std::initializer_list` is not a value type - do not return from functions
- Devices and handles use DISALLOW_COPY but allow move
- Each handle maintains state for both USER and SYSTEM slots
- Only changes to the active slot affect device state
- Manager provides convenient iteration over all devices
- Sanitizer enabled: address, leak, undefined behavior detection
