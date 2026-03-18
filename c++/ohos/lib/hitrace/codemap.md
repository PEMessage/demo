# lib/hitrace/

HiTrace is OpenHarmony's distributed tracing framework for performance analysis and debugging across processes, devices, and the distributed system.

---

## Responsibility

HiTrace enables **end-to-end distributed tracing** of requests across:
- **Thread-to-thread** communication within a process
- **Process-to-process** communication via IPC/RPC
- **Device-to-device** communication in distributed scenarios

### Core Responsibilities

| Component | Responsibility |
|-----------|--------------|
| **Trace Chain** (`hitracechainc.c`, `hitracechain.cpp`) | Manages trace lifecycle (begin/end), thread-local trace IDs, span creation |
| **Trace ID** (`hitraceid.cpp`, `hitracechainc.h`) | Represents unique identifiers with chainId, spanId, parentSpanId (16 bytes) |
| **Tracepoint Logging** (`hitracechainc.c`) | Logs communication events (CS/CR/SS/SR) with trace context |
| **Trace Meter** (`hitrace_meter.cpp`) | Outputs trace events to kernel trace_marker or app-specific trace files |
| **Dynamic Buffer** (`dynamic_buffer.cpp`) | Calculates optimal trace buffer sizes per-CPU based on load |

---

## Design

### Trace ID Structure

The `HiTraceIdStruct` is a 16-byte structure (bit-packed):

```
┌─────────────────────────────────────────────────────────────┐
│  First 8 bytes:                                             │
│  ├─ valid: 1 bit      - ID validity                         │
│  ├─ ver: 3 bits       - Version (currently 1)               │
│  └─ chainId: 60 bits  - Unique trace chain identifier       │
├─────────────────────────────────────────────────────────────┤
│  Second 8 bytes:                                            │
│  ├─ flags: 12 bits    - Trace behavior flags                │
│  ├─ spanId: 26 bits   - Current span identifier             │
│  └─ parentSpanId: 26 bits - Parent span identifier          │
└─────────────────────────────────────────────────────────────┘
```

**Chain ID Generation:**
- Combines device ID, CPU ID, timestamp (seconds + microseconds)
- Device ID is lazily initialized with random value per process

### Trace Chain Hierarchy Pattern

```
Device A (Root)                              Device B (Child)
┌─────────────┐                             ┌─────────────┐
│  Service A  │──── CS/CR/SR/SS ──────────> │  Service B  │
│  chainId=X  │    (distributed call)       │  chainId=X  │
│  spanId=H1  │                             │  spanId=H2  │
│  parent=0   │                             │  parent=H1  │
└─────────────┘                             └─────────────┘
```

### Span Creation Algorithm

```cpp
// Child span ID generation (BKDRHash-based)
HiTraceId CreateSpan() {
    uint32_t hashData[5] = {
        deviceId,           // Current device
        parentSpanId,       // Parent's span ID
        currentSpanId,      // Current span ID
        tv.tv_sec,          // Timestamp seconds
        tv.tv_usec          // Timestamp microseconds
    };
    newSpanId = HashFunc(hashData, sizeof(hashData));
    newParentSpanId = currentSpanId;
}
```

### Tracepoint Types

| Type | Code | Direction | Usage |
|------|------|-----------|-------|
| **CS** | Client Send | Outbound | Client sending request |
| **CR** | Client Receive | Inbound | Client receiving response |
| **SS** | Server Send | Outbound | Server sending response |
| **SR** | Server Receive | Inbound | Server receiving request |
| **GENERAL** | General | - | Informational tracepoint |

### Communication Modes

| Mode | Enum | Scope |
|------|------|-------|
| DEFAULT | `HITRACE_CM_DEFAULT` | Unspecified |
| THREAD | `HITRACE_CM_THREAD` | Thread-to-thread |
| PROCESS | `HITRACE_CM_PROCESS` | Process-to-process (IPC) |
| DEVICE | `HITRACE_CM_DEVICE` | Device-to-device (distributed) |

### Trace Flags (Behavior Control)

| Flag | Description |
|------|-------------|
| `HITRACE_FLAG_INCLUDE_ASYNC` | Include async calls in trace |
| `HITRACE_FLAG_DONOT_CREATE_SPAN` | Disable automatic child span creation |
| `HITRACE_FLAG_TP_INFO` | Output tracepoint information to logs |
| `HITRACE_FLAG_NO_BE_INFO` | Suppress begin/end info logging |
| `HITRACE_FLAG_DONOT_ENABLE_LOG` | Don't append trace ID to HiLog |
| `HITRACE_FLAG_FAULT_TRIGGER` | Mark trace as fault-triggered |
| `HITRACE_FLAG_D2D_TP_INFO` | Only log device-to-device tracepoints |

### Thread-Local Storage Design

Trace context is stored in thread-local storage:

```c
// Single thread-local instance per thread
static __thread HiTraceIdStructInner g_hiTraceId = {{0}, {0}};

// Provides:
// - Thread isolation (no locks needed)
// - Automatic inheritance within thread
// - Save/restore for context switching
```

### Trace Meter Tags

Over 60 subsystem tags for filtering (defined in `hitrace_meter.h`):
- `HITRACE_TAG_OHOS` - Generic OHOS tracing
- `HITRACE_TAG_RPC` - RPC/IPC tracing
- `HITRACE_TAG_DSOFTBUS` - Distributed softbus
- `HITRACE_TAG_ARK` - Ark runtime
- `HITRACE_TAG_APP` - Application tracing
- etc.

---

## Flow

### 1. Trace Initialization Flow

```
┌─────────────┐     ┌─────────────────┐     ┌─────────────────────┐
│ Application │────>│ HiTraceChain::  │────>│ Generate Chain ID   │
│    Code     │     │    Begin()      │     │ (device+cpu+time)   │
└─────────────┘     └─────────────────┘     └─────────────────────┘
                                                    │
                                                    ▼
                                           ┌─────────────────────┐
                                           │ Store in thread-    │
                                           │ local g_hiTraceId   │
                                           └─────────────────────┘
```

### 2. Distributed Trace Propagation Flow

```
Process A                                          Process B
┌─────────────┐                              ┌─────────────┐
│ Begin()     │                              │             │
│       │     │      Serialize ID            │             │
│       ▼     │   ┌──────────────┐           │             │
│  CreateSpan()│  │ ToBytes()    │           │             │
│       │     │   │ 16 bytes     │ ────────> │ SetId()     │
│       │     │   └──────────────┘  (IPC)    │       │     │
│  Tracepoint()│                              │       ▼     │
│  (CS send)  │                              │  Tracepoint()│
│       │     │      Response                │  (SR recv)  │
│       ▼     │   <────────────────────────  │       │     │
│  Tracepoint()│                              │       ▼     │
│  (CR recv)  │                              │  Tracepoint()│
│       │     │                              │  (SS send)  │
│       ▼     │                              │             │
│  End()      │                              │  End()      │
└─────────────┘                              └─────────────┘
```

### 3. Trace Meter Output Flow

```
┌─────────────┐     ┌──────────────────────┐     ┌─────────────────────┐
│ App Code    │     │ Trace Meter          │     │ Output Destinations │
│ (C/C++)     │────>│ (hitrace_meter.cpp)  │────>│                     │
└─────────────┘     └──────────────────────┘     ├─────────────────────┤
                          │                      │ • Kernel trace_marker│
                          ▼                      │   (/sys/kernel/.../) │
                   ┌──────────────┐              │ • App trace files    │
                   │ Check tag    │              │   (*.trace)          │
                   │ enabled?     │              └─────────────────────┘
                   └──────────────┘
                          │
                          ▼
                   ┌──────────────┐
                   │ Format trace │
                   │ record:      │
                   │ B|E|S|F|C    │
                   └──────────────┘
                          │
                          ▼
                   ┌──────────────┐
                   │ Write to fd  │
                   └──────────────┘
```

### 4. Typical Usage Flow (C++)

```cpp
#include "hitrace/tracechain.h"

// 1. Begin trace - creates root span with chainId
HiTraceId id = HiTraceChain::Begin("operation", HITRACE_FLAG_INCLUDE_ASYNC);

// 2. Log client send tracepoint
HiTraceChain::Tracepoint(HITRACE_CM_DEVICE, HITRACE_TP_CS, id, "sending");

// 3. Create child span for distributed call
HiTraceId childSpan = HiTraceChain::CreateSpan();

// 4. Serialize and send across network
uint8_t buf[16];
childSpan.ToBytes(buf, sizeof(buf));
// Send buf to remote...

// On remote side:
HiTraceId remoteId(buf, sizeof(buf));
HiTraceChain::SetId(remoteId);

// 5. Log server receive tracepoint
HiTraceChain::Tracepoint(HITRACE_CM_DEVICE, HITRACE_TP_SR, remoteId, "received");

// 6. Process and respond...

// 7. Log server send tracepoint  
HiTraceChain::Tracepoint(HITRACE_CM_DEVICE, HITRACE_TP_SS, remoteId, "responding");

// 8. End trace
HiTraceChain::End(id);
```

### 5. Trace Meter Scoped Helper Flow

```cpp
// Scoped trace - automatic begin/end
{
    HITRACE_METER(HITRACE_TAG_OHOS);  // Uses __func__ as name
    // ... code being traced ...
} // Automatic FinishTrace on scope exit

// Equivalent to:
StartTrace(HITRACE_TAG_OHOS, __func__);
// ... code ...
FinishTrace(HITRACE_TAG_OHOS);
```

---

## Integration

### Build Dependencies

From `CMakeLists.txt`:

```cmake
target_link_libraries(hitrace PUBLIC
    libsec       # Secure C library (memcpy_s, etc.)
    hilog        # HiLog logging framework
    nocopyable   # Non-copyable utility class
    cjson        # JSON parsing for trace config
    syspara      # System parameter access
)
```

### Module Directory Structure

```
lib/hitrace/
├── CMakeLists.txt                    # Build configuration
│
├── common/                           # Common definitions
│   ├── common_define.h              # Trace paths, constants
│   └── hitrace_define.h             # Trace modes, error codes
│
├── frameworks/                       # Core implementations
│   ├── include/
│   │   ├── dynamic_buffer.h         # Dynamic trace buffer sizing
│   │   └── hitrace_meter_wrapper.h  # C wrapper declarations
│   │
│   └── native/                      # Implementation files
│       ├── hitracechain.cpp         # C++ chain API (thin wrapper)
│       ├── hitracechainc.c          # C chain API (core logic)
│       ├── hitracechain_inner.h     # Internal interfaces
│       ├── hitraceid.cpp            # HiTraceId class operations
│       ├── dynamic_buffer.cpp       # Per-CPU buffer calculation
│       └── c_wrapper/               # Additional C wrappers
│
├── interfaces/                       # Public APIs
│   └── native/innerkits/
│       ├── include/
│       │   ├── hitrace/
│       │   │   ├── hitracechain.h   # C++ chain API
│       │   │   ├── hitracechainc.h  # C API + data structures
│       │   │   ├── hitraceid.h      # HiTraceId C++ class
│       │   │   ├── trace.h          # Convenience header
│       │   │   └── tracechain.h     # Convenience header
│       │   │
│       │   └── hitrace_meter/
│       │       ├── hitrace_meter.h  # Main meter API (C++)
│       │       └── hitrace_meter_c.h # C meter API
│       │
│       └── src/
│           ├── hitrace_meter.cpp    # Meter implementation
│           ├── hitrace_meter_c.c    # C meter wrappers
│           ├── hitrace_meter_wrapper.cpp # C/C++ bridge
│           └── hitrace_meter_mock.cpp    # Test mocks
│
├── utils/                            # Utility functions
│   ├── common_utils.cpp             # String/int conversions
│   ├── common_utils.h
│   ├── trace_file_utils.cpp         # File I/O operations
│   ├── trace_file_utils.h
│   ├── file_ageing_utils.cpp        # File cleanup
│   └── trace_json_parser.cpp        # JSON parsing
│
├── codemap.md                       # This file
├── readme.md                        # Documentation
└── keep.c                           # Placeholder
```

### Key API Entry Points

| Layer | Header | Key Classes/Functions |
|-------|--------|----------------------|
| C++ Chain | `hitrace/tracechain.h` | `HiTraceChain`, `HiTraceId` |
| C Chain | `hitrace/hitracechainc.h` | `HiTraceChainBegin()`, `HiTraceChainEnd()`, `HiTraceChainCreateSpan()` |
| C++ Meter | `hitrace_meter/hitrace_meter.h` | `StartTrace()`, `FinishTrace()`, `HITRACE_METER` macro |
| C Meter | `hitrace_meter/hitrace_meter_c.h` | `HiTraceStartTrace()`, `HiTraceFinishTrace()` |

### HiLog Integration

HiTrace integrates with HiLog for contextual logging:

```c
// Registration (in hitracechainc.c)
static void __attribute__((constructor)) HiTraceChainInit(void) {
    HiLogRegisterGetIdFun(HiTraceChainGetInfo);
}

// Result: Log messages include trace context
// Format: <mode,type,[chainId,spanId,parentSpanId]> message
```

### Kernel trace_marker Integration

Trace events are written to Linux kernel's `trace_marker`:

```
/sys/kernel/debug/tracing/trace_marker
/sys/kernel/tracing/trace_marker
```

Format follows Android systrace convention:
```
B|pid|name     - Begin trace
E|pid          - End trace
S|pid|name|tid - Async begin
F|pid|name|tid - Async end
C|pid|name|val - Counter
```

### System Parameter Dependencies

| Parameter | Purpose |
|-----------|---------|
| `debug.hitrace.tags.enableflags` | Bitmask of enabled trace tags |
| `debug.hitrace.app_pid` | PID filter for app tracing |
| `persist.hitrace.level.threshold` | Minimum trace level to output |

### Typical Consumer Modules

- **DSoftBus**: Distributed communication tracing
- **RPC**: IPC/RPC call tracing  
- **Ark Runtime**: JavaScript/ArkTS execution tracing
- **Ability Manager**: Ability lifecycle tracing
- **Window Manager**: UI/window operation tracing
- **Applications**: Custom app-level tracing
