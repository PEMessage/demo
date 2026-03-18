# lib/syspara/

OpenHarmony System Parameters Library - Provides unified access to system-wide configuration parameters.

## Responsibility

The syspara library serves as the central access point for system parameters in OpenHarmony:

- **Parameter Storage Management**: Stores and retrieves key-value pairs for system configuration
- **Hardware/Device Information**: Provides device-specific information (model, manufacturer, serial, etc.)
- **OS Version Information**: Manages OS version, API levels, build information
- **Access Control**: Enforces permission checks for parameter read/write operations
- **Persistence**: Supports persistent parameters that survive reboots
- **Event Notification**: Supports watching for parameter changes
- **Multiple API Styles**: Offers C, C++, and various wrapper interfaces

Key constants defined:
- `PARAM_NAME_LEN_MAX`: 96 bytes max for parameter names
- `PARAM_VALUE_LEN_MAX`: 96 bytes max for parameter values
- `PARAM_CONST_VALUE_LEN_MAX`: 4096 bytes for constant values

## Design

### Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│  Application Layer (C++ wrappers)                           │
│  parameters.h, param_wrapper.h → param_wrapper.cpp          │
├─────────────────────────────────────────────────────────────┤
│  Public C API Layer                                         │
│  parameter.h → parameter.c                                  │
├─────────────────────────────────────────────────────────────┤
│  Internal Helper Layer                                      │
│  param_comm.h → param_comm.c                                │
├─────────────────────────────────────────────────────────────┤
│  System Service Interface                                   │
│  init_param.h, param_init.h, sys_param.h                    │
├─────────────────────────────────────────────────────────────┤
│  Version Utilities                                          │
│  sysversion.h → sysversion.c                                │
└─────────────────────────────────────────────────────────────┘
```

### Storage Pattern

Parameters are stored in a **shared memory workspace** (`WorkSpace` in sys_param.h):

```c
typedef struct WorkSpace_ {
    unsigned int flags;
    MemHandle memHandle;
    ParamTrieHeader *area;        // Shared memory header
    ATOMIC_UINT32 rwSpaceLock;    // Atomic lock for synchronization
    uint32_t spaceSize;
    uint32_t spaceIndex;
    ParamRWMutex rwlock;
    char fileName[0];
} WorkSpace;
```

The shared memory contains:
- `ParamTrieHeader`: Metadata including commit IDs, node counts, data size
- **Trie structure**: Efficient prefix-based parameter lookup
- **Atomic commit IDs**: Track changes for cache invalidation

### Caching Strategy

**CachedParameter** provides optimized read access:
- Stores parameter handle and cached value
- Uses atomic commit ID checking to detect changes
- Avoids repeated lookups for frequently accessed parameters

```c
typedef struct CachedParameter_ {
    struct WorkSpace_ *workspace;
    long long spaceCommitId;      // Last known commit ID
    uint32_t dataCommitId;
    char *paramValue;
    ...
} CachedParameter;
```

### Property Access Pattern

Static caching for frequently-accessed properties:
```c
const char *GetProductModel(void) {
    static const char *productModel = NULL;
    return GetProperty("const.product.model", &productModel);
}
```

Properties starting with `const.` are read-only and cached after first access.

### Error Code Mapping

Internal error codes (`PARAM_CODE_*`) are mapped to public error codes:
- `EC_INVALID` (-9): Invalid parameter
- `SYSPARAM_NOT_FOUND` (-14700101): Parameter not found
- `SYSPARAM_PERMISSION_DENIED` (-14700103): Access denied
- `SYSPARAM_SYSTEM_ERROR` (-14700104): Internal error

## Flow

### Read Flow

```
GetParameter(key, def, value, len)
    │
    ▼
GetParameter_(key, def, value, len)  [param_comm.c]
    │
    ├──► SystemGetParameter(key, NULL, &size)  [Get size first]
    │
    └──► SystemGetParameter(key, value, &size) [Get actual value]
              │
              ▼
         Shared memory lookup via trie
              │
              ├──► Found: Return value
              └──► Not found: Return default
```

### Write Flow

```
SetParameter(key, value)
    │
    ▼
SystemSetParameter(key, value)
    │
    ├──► Permission check (DAC/SELinux)
    │
    ├──► Update shared memory
    │
    ├──► Increment commit ID (atomic)
    │
    └──► Notify watchers (if any)
```

### Watch/Wait Flow

```
WatchParameter(keyPrefix, callback, context)
    │
    ▼
Register callback for parameter changes
    │
    └──► On change: callback(key, value, context)

WaitParameter(key, value, timeout)
    │
    ▼
Block until parameter equals expected value
    │
    ├──► Check current value
    ├──► Wait for change notification (futex/condition)
    └──► Timeout or match → return
```

### Parameter Traversal Flow

```
SystemTraversalParameter(prefix, callback, cookie)
    │
    ▼
Traverse trie nodes matching prefix
    │
    └──► For each match: callback(handle, cookie)
```

## Integration

### Dependencies

**Build Dependencies** (from CMakeLists.txt):
- `misc`: Utility functions
- `libsec`: Security library
- `init_utils`: Init process utilities

**Header Dependencies**:
- `beget_ext.h`: Base initialization extensions
- `securec.h`: Safe C library functions
- `init_param.h`: Init service parameter interface
- `cJSON.h`: JSON parsing (for trigger support)

### API Surface

#### C Public API (parameter.h)

**Core Operations**:
```c
int GetParameter(const char *key, const char *def, char *value, uint32_t len);
int SetParameter(const char *key, const char *value);
int WaitParameter(const char *key, const char *value, int timeout);
```

**Watching**:
```c
typedef void (*ParameterChgPtr)(const char *key, const char *value, void *context);
int WatchParameter(const char *keyPrefix, ParameterChgPtr callback, void *context);
int RemoveParameterWatcher(const char *keyPrefix, ParameterChgPtr callback, void *context);
```

**Handle-based Access** (efficient for repeated access):
```c
uint32_t FindParameter(const char *key);
int GetParameterValue(uint32_t handle, char *value, uint32_t len);
```

**Device/OS Info**:
```c
const char *GetDeviceType(void);
const char *GetProductModel(void);
const char *GetOSFullName(void);
int GetSdkApiVersion(void);
```

#### C++ API (parameters.h, param_wrapper.h)

```cpp
std::string GetParameter(const std::string& key, const std::string& def);
bool GetBoolParameter(const std::string& key, bool def);
bool SetParameter(const std::string& key, const std::string& value);
```

### Service Interface (init_param.h)

Used by the init process for parameter service management:

```c
int InitParamService(void);
int StartParamService(void);
void StopParamService(void);
int LoadDefaultParams(const char *fileName, uint32_t mode);
int LoadPersistParams(void);
```

### Common Parameter Keys

The library accesses these system-defined parameters:

| Function | Parameter Key |
|----------|--------------|
| GetDeviceType | `const.product.devicetype`, `const.build.characteristics` |
| GetProductModel | `const.product.model` |
| GetManufacture | `const.product.manufacturer` |
| GetOSFullName | `const.ohos.fullname` |
| GetSdkApiVersion | `const.ohos.apiversion` |
| GetBrand | `const.product.brand` |
| GetSerial | `const.product.serial` |
| GetDisplayVersion | `const.product.software.version` |

## File Structure

| File | Purpose |
|------|---------|
| `include/parameter.h` | Main public C API |
| `include/parameters.h` | C++ template API |
| `include/param_wrapper.h` | C++ wrapper functions |
| `include/init_param.h` | Init service interface |
| `include/param_init.h` | Parameter traversal API |
| `include/sys_param.h` | Shared memory structures |
| `include/sysversion.h` | Version number API |
| `include/sysparam_errno.h` | Error code definitions |
| `src/parameter.c` | C API implementation |
| `src/param_wrapper.cpp` | C++ API implementation |
| `src/param_comm.c` | Common utilities, property caching |
| `src/sysversion.c` | Version parsing from fullname |
