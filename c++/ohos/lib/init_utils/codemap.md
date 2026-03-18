# lib/init_utils/

## Responsibility

Provides foundational C utilities for the init process and early system startup. Includes hash maps, logging, process utilities, file operations, and string processing - designed for use before full C++ runtime is available.

## Design

### Core Modules

#### 1. Hash Map (init_hashmap.c / init_hashmap.h)
- Generic hash map implementation for C
- Separate chaining collision resolution
- User-provided hash and compare functions
- Key APIs: `OH_HashMapCreate`, `OH_HashMapAdd`, `OH_HashMapGet`, `OH_HashMapRemove`
- Configurable bucket count (16 to 1024)

#### 2. Utilities (init_utils.c / init_utils.h)
- **User/Group Management**: `DecodeUid()`, `DecodeGid()` - resolve names to IDs
- **File Operations**: `ReadFileToBuf()`, `ReadFileData()`, `MakeDirRecursive()`
- **String Processing**: `SplitString()`, `SplitStringExt()`, `StringToInt()`, `StringReplaceChr()`
- **Timing**: `InitDiffTime()`, `GetUptimeInMicroSeconds()`
- **Boot Parameters**: `GetParameterFromCmdLine()`, `GetExactParameterFromCmdLine()`
- **Array/Dictionary**: `OH_StrArrayGetIndex()`, `OH_StrDictGet()` - generic lookup utilities

#### 3. Logging (init_log.c / init_log.h, init_commlog.c)
- Multiple log destinations: HiLog, dmesg (/dev/kmsg), stderr, file
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Compile-time and runtime level filtering
- Helper macros: `INIT_LOGI`, `INIT_LOGE`, `INIT_ERROR_CHECK`, etc.
- Pluggable log handler via `SetInitCommLog()`

#### 4. List (list.c)
- Doubly-linked list implementation
- APIs: `OH_ListInit`, `OH_ListAddTail`, `OH_ListRemove`, `OH_ListTraversal`
- Ordered insertion with comparator support
- Safe traversal with reverse order and error-stop flags

### Extension Header (beget_ext.h)
- API visibility macros: `INIT_PUBLIC_API`, `INIT_LOCAL_API`
- Domain and label definitions for logging
- Additional check macros: `BEGET_ERROR_CHECK`, `BEGET_LOGI`, etc.

## Flow

1. **Initialization**: Components call `OH_HashMapCreate`, `OH_ListInit`, or `OpenLogDevice`
2. **Normal Operation**: Use hash maps for key-value storage, lists for collections, logging for diagnostics
3. **Cleanup**: `OH_HashMapDestory`, `OH_ListRemoveAll` for resource cleanup
4. **Logging Flow**: `StartupLog()` → registered handler → destination (HiLog/dmesg/stderr)

## Integration

**Dependencies:**
- lib/misc (for error patterns)
- lib/libsec (secureC functions)

**Used By:**
- init process (system startup)
- Early boot components
- Components needing C-based utilities

**CMake:** Static library with C and C++ sources

**Note:** This library is specifically designed for early boot before full C++ infrastructure is available.
