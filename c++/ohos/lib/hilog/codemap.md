# HiLog Library Code Map

## 1. Responsibility

### Purpose
HiLog is OpenHarmony's high-performance logging framework that provides a unified logging service for system components and applications. It offers both C and C++ APIs with features like log level control, privacy protection, flow control, and trace integration.

### Key Features
- **Multi-level Logging**: Debug, Info, Warn, Error, Fatal levels
- **Log Types**: APP (0x0-0xFFFF), CORE/INIT (0xD000000-0xD0FFFFF), KMSG, ONLY_PRERELEASE
- **Privacy Protection**: `{public}`/`{private}` format specifiers to mask sensitive data
- **Flow Control**: Process-level quota management to prevent log flooding
- **Trace Integration**: Support for distributed tracing (chainId, spanId, parentSpanId)
- **Cross-Platform**: Supports Linux, Windows, macOS, and OpenHarmony platforms

### Module Boundaries
- **Input**: Log requests from applications and system services via C/C++ APIs
- **Output**: Log messages sent to hilogd daemon via Unix domain socket (on OHOS) or stdout (on other platforms)

---

## 2. Design

### Architecture Overview
```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │ C++ HiLog   │  │ C HiLogPrint│  │ C++ HiLogBasePrint      │  │
│  │ Class API   │  │ Function    │  │ (Socket-based)          │  │
│  └──────┬──────┘  └──────┬──────┘  └─────────────────────────┘  │
└─────────┼────────────────┼───────────────────────────────────────┘
          │                │
          ▼                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Interface Layer                            │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    HiLogPrintArgs                          │   │
│  │              (hilog_printf.cpp - Core Implementation)      │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Transport Layer                            │
│  ┌─────────────────┐  ┌─────────────────────────────────────┐   │
│  │ vsnprintf_p     │  │ Unix Domain Socket (hilogd)         │   │
│  │ (Privacy-aware  │  │ or stdout (Linux/Windows fallback)  │   │
│  │  formatting)    │  │                                     │   │
│  └─────────────────┘  └─────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Key Classes and Structures

#### HiLogLabel (log_cpp.h:33-37)
```cpp
struct HiLogLabel {
    LogType type;        // LOG_APP, LOG_CORE, LOG_INIT, LOG_KMSG
    unsigned int domain; // 0x0-0xFFFF (APP) or 0xD000000-0xD0FFFFF (OS)
    const char *tag;     // Module identifier (max 31 chars)
};
```
- Used to categorize log messages by type, domain, and tag
- Domain helps filter and route logs to appropriate handlers

#### HiLog Class (log_cpp.h:40-58)
```cpp
class HiLog final {
public:
    static int Debug(const HiLogLabel &label, const char *fmt, ...);
    static int Info(const HiLogLabel &label, const char *fmt, ...);
    static int Warn(const HiLogLabel &label, const char *fmt, ...);
    static int Error(const HiLogLabel &label, const char *fmt, ...);
    static int Fatal(const HiLogLabel &label, const char *fmt, ...);
};
```
- Static methods for each log level
- Uses macro HILOG_VA_ARGS_PROCESS to forward to HiLogPrintArgs

#### HilogMsg (hilog_base.h:31-44)
```cpp
typedef struct __attribute__((__packed__)) {
    uint16_t len;        // Total message length
    uint16_t version : 3;
    uint16_t type : 4;   // APP, CORE, INIT, KMSG, etc.
    uint16_t level : 3;  // DEBUG, INFO, WARN, ERROR, FATAL
    uint16_t tagLen : 6; // Include null terminator
    uint32_t tv_sec;     // Timestamp seconds
    uint32_t tv_nsec;    // Timestamp nanoseconds
    uint32_t mono_sec;   // Monotonic timestamp
    uint32_t pid;        // Process ID
    uint32_t tid;        // Thread ID
    uint32_t domain;     // Log domain
    char tag[];          // Variable-length tag
} HilogMsg;
```
- Binary protocol message header sent to hilogd
- Packed structure for efficient serialization

#### LogContent & LogFormat (log_print.h:23-43)
```cpp
struct LogContent {
    uint8_t level, type;
    uint32_t pid, tid, domain;
    uint32_t tv_sec, tv_nsec, mono_sec;
    const char *tag, *log;
};

struct LogFormat {
    bool colorful;
    FormatTime timeFormat;      // TIME, EPOCH, MONOTONIC
    FormatTimeAccu timeAccuFormat; // MSEC, USEC, NSEC
    bool year, zone, wrap;
};
```
- Used for formatted log output on non-OHOS platforms

### Design Patterns

1. **Facade Pattern**: HiLog class provides simplified C++ interface hiding complex internals
2. **Adapter Pattern**: Platform-specific implementations (PrintLog for Linux/Windows vs socket for OHOS)
3. **Strategy Pattern**: Different formatting strategies via LogFormat
4. **Singleton Pattern**: Global socket connection (g_socketFd) managed with atomic operations
5. **Template Pattern**: KVMap for type-safe key-value mappings (log_utils.h:24-69)

---

## 3. Flow

### Log Request Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│ Step 1: Application Call                                            │
│                                                                     │
│   HiLog::Info(label, "User %s logged in", username);                │
│   or                                                                │
│   HILOG_INFO(LOG_APP, "User %{public}s logged in", username);       │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Step 2: Macro Expansion (C API)                                     │
│                                                                     │
│   HILOG_IMPL(LOG_APP, LOG_INFO, LOG_DOMAIN, LOG_TAG, fmt, ...)      │
│   → HiLogPrint(type, level, domain, tag, fmt, ...)                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Step 3: Core Processing (hilog_printf.cpp:239-375)                  │
│                                                                     │
│   HiLogPrintArgs(type, level, domain, tag, fmt, va_list)            │
│   ├─▶ HiLogIsLoggable() - Check level/domain filters               │
│   ├─▶ Get timestamps (CLOCK_REALTIME, CLOCK_MONOTONIC)             │
│   ├─▶ Get trace IDs via g_registerFunc (if registered)             │
│   ├─▶ vsnprintfp_s() - Format with privacy handling                 │
│   ├─▶ HiLogFlowCtrlProcess() - Rate limiting (if APP type)         │
│   └─▶ Send to socket or PrintLog()                                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    ▼                               ▼
┌──────────────────────────────┐      ┌──────────────────────────────┐
│ Step 4a: OHOS Platform       │      │ Step 4b: Linux/Windows       │
│                              │      │                              │
│   HilogWriteLogMessage()     │      │   PrintLog()                 │
│   → Unix domain socket       │      │   → LogPrintWithFormat()     │
│   → /dev/unix/socket/hilogInput│    │   → std::cout with colors    │
└──────────────────────────────┘      └──────────────────────────────┘
```

### Privacy Protection Flow

```cpp
// Format string: "User %{public}s, SSN: %{private}s"
// Input: username="alice", ssn="123-45-6789"

vsnprintfp_s(buf, MAX_LOG_LEN, MAX_LOG_LEN - 1, 
             HiLogIsPrivacyOn(), fmt, ap);

// Output when privacy ON:  "User alice, SSN: <private>"
// Output when privacy OFF: "User alice, SSN: 123-45-6789"
```

### Flow Control Flow

```cpp
HiLogFlowCtrlProcess(len, ts)  // hilog_printf.cpp:140-170
├── Get process quota from configuration
├── Check if current period (1 second) quota exceeded
├── If exceeded: increment dropped count, return -1 (drop)
├── If new period: return dropped count, reset counters
└── If under quota: add len to sum, return 0 (allow)
```

### Trace Integration Flow

```cpp
// Registration (optional)
HiLogRegisterGetIdFun(myTraceFunc);  // Sets g_registerFunc

// At log time:
if (g_registerFunc != nullptr) {
    func(&chainId, &flag, &spanId, &parentSpanId);
    // Prepend [chainId, spanId, parentSpanId] to log message
}
```

---

## 4. Integration

### Dependencies

#### Build Dependencies (CMakeLists.txt)
- `libsec`: Security library for secure string operations
- `securec.h`: Secure C library functions
- Standard C/C++ libraries: `<cstdarg>`, `<ctime>`, `<mutex>`, `<atomic>`

#### Platform-Specific Dependencies
| Platform | Headers | Functions |
|----------|---------|-----------|
| Linux | `<unistd.h>`, `<sys/syscall.h>` | `syscall(SYS_gettid)` |
| Windows | `<windows.h>` | `GetCurrentProcessId()`, `GetCurrentThreadId()` |
| macOS | `<pthread.h>` | `pthread_threadid_np()` |
| OHOS | `<sys/un.h>`, `<sys/socket.h>` | Unix domain sockets |

### API Usage Examples

#### C++ API (Recommended)
```cpp
#include "hilog/log.h"

// Define label once
static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD000000, "MyModule"};

// Use macros
HiLogInfo(LABEL, "Processing item %{public}d", itemId);
HiLogError(LABEL, "Failed to open file: %{public}s", filename);
```

#### C API
```c
#include "hilog/log_c.h"

// Set domain and tag via macros
#define LOG_DOMAIN 0xD000000
#define LOG_TAG "MyModule"

HILOG_INFO(LOG_CORE, "Processing item %{public}d", itemId);
```

#### Setting Log Level
```cpp
// Set minimum log level for the application
HiLogSetAppMinLogLevel(LOG_WARN);  // Only WARN and above
```

#### Custom Log Callback
```cpp
void MyLogHandler(LogType type, LogLevel level, unsigned int domain,
                  const char* tag, const char* msg) {
    // Custom processing
}

LOG_SetCallback(MyLogHandler);
```

### Integration with hilogd (OHOS)

```
┌─────────────┐    Unix Domain Socket     ┌─────────────┐
│  libhilog   │ ─── /dev/unix/socket/ ──→ │   hilogd    │
│  (client)   │      hilogInput           │   (daemon)  │
└─────────────┘                           └─────────────┘
                                                   │
                                                   ▼
                                          ┌─────────────┐
                                          │  Log Buffer │
                                          │  (ring)     │
                                          └─────────────┘
```

### File Structure

```
lib/hilog/
├── CMakeLists.txt                    # Build configuration
├── frameworks/
│   ├── include/
│   │   └── hilog_inner.h            # Internal C API (HiLogPrintArgs)
│   └── libhilog/
│       ├── hilog.cpp                # C++ HiLog class implementation
│       ├── hilog_printf.cpp         # Core HiLogPrintArgs implementation
│       ├── hilog_base.c             # Low-level socket client (base)
│       ├── include/
│       │   ├── hilog_base.h         # HilogMsg structure
│       │   ├── hilog_cmd.h          # IOCTL commands
│       │   └── hilog_common.h       # Common constants
│       ├── utils/
│       │   ├── log_print.cpp        # Formatted output for non-OHOS
│       │   ├── log_utils.cpp        # Utility functions
│       │   └── include/
│       │       ├── log_print.h      # LogContent/LogFormat
│       │       ├── log_utils.h      # KVMap, converters
│       │       └── log_timestamp.h  # LogTimeStamp class
│       └── vsnprintf/
│           ├── vsnprintf_s_p.c      # Privacy-aware formatting
│           └── include/
│               └── vsnprintf_s_p.h
└── interfaces/native/innerkits/include/
    ├── hilog/
    │   ├── log.h                    # Main include (C + C++)
    │   ├── log_c.h                  # C API definitions
    │   └── log_cpp.h                # C++ HiLog class
    ├── hilog_base/
    │   └── log_base.h               # Base C API
    └── hilog_trace.h                # Trace registration API
```

---

## 5. Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| MAX_LOG_LEN | 4096 | Maximum log message length |
| MAX_TAG_LEN | 32 | Maximum tag length |
| DOMAIN_APP_MIN | 0x0 | Minimum app domain |
| DOMAIN_APP_MAX | 0xFFFF | Maximum app domain |
| DOMAIN_OS_MIN | 0xD000000 | Minimum OS domain |
| DOMAIN_OS_MAX | 0xD0FFFFF | Maximum OS domain |
| LOG_DEBUG | 3 | Debug log level |
| LOG_INFO | 4 | Info log level |
| LOG_WARN | 5 | Warning log level |
| LOG_ERROR | 6 | Error log level |
| LOG_FATAL | 7 | Fatal log level |

---

## 6. Thread Safety

- **Socket Management**: Uses atomic compare-and-swap (CAS) for lazy socket initialization
- **Fatal Message Buffer**: Protected by `std::mutex` for FATAL level logs
- **Format Printing**: Uses thread-local stream handling in `log_print.cpp`
- **Flow Control**: Uses atomic operations for counters in multi-threaded environment

---

## 7. Security Considerations

1. **Privacy Markers**: `{public}` and `{private}` format specifiers control data exposure
2. **Buffer Overflow Protection**: Uses `vsnprintfp_s` with bounds checking
3. **Secure String Functions**: Uses `memcpy_s`, `snprintf_s` from securec library
4. **Domain Validation**: Checks domain ranges to prevent invalid log injections
