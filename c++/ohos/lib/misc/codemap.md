# lib/misc/

## Responsibility

Core utility library providing fundamental building blocks used throughout OpenHarmony. Contains error code definitions, thread-safe data structures, singleton patterns, logging infrastructure, and various helper utilities.

## Design

### Error System
- **errors.h**: System-wide error code layout and common error definitions
  - 32-bit error code format: [Reserved 3bits | Subsystem 8bits | Module 5bits | Code 16bits]
  - `ErrCodeOffset()` macro for subsystem error base calculation
  - Common errors: `ERR_OK`, `ERR_NO_MEMORY`, `ERR_INVALID_VALUE`, etc.
  - `SUCCEEDED()` / `FAILED()` macros for result checking

- **common_errors.h**: Module identifiers for commonlibrary subsystem
  - `MODULE_DEFAULT`, `MODULE_TIMER`, `MODULE_MAPPED_FILE`, etc.

### Thread-Safe Data Structures
- **safe_queue.h**: `SafeQueue<T>` and `SafeStack<T>` templates
  - Abstract base `SafeQueueInner<T>` with mutex-protected operations
  - Virtual `DoPush()` / `DoPop()` for queue vs stack behavior

- **safe_block_queue.h**: Blocking queue with condition variables
  - `SafeBlockQueue<T>`: Blocking `Push()`/`Pop()`, non-blocking `PushNoWait()`/`PopNotWait()`
  - `SafeBlockQueueTracking<T>`: Adds task tracking with `OneTaskDone()` / `Join()`

- **safe_map.h**: `SafeMap<K,V>` with fine-grained locking
  - Insert, Find, Erase, Iterate operations
  - `ChangeValueByLambda()` for atomic updates
  - Clone-based copy constructor for thread-safe copying

### Singleton Patterns
- **singleton.h**: Three singleton variants
  - `DelayedSingleton<T>`: Lazy init with `std::shared_ptr`, thread-safe double-checked locking
  - `DelayedRefSingleton<T>`: Lazy init with raw pointer, returns reference
  - `Singleton<T>`: Eager init, simplest implementation
  - Helper macros: `DECLARE_DELAYED_SINGLETON`, `DECLARE_SINGLETON`

### Utilities
- **utils_log.h**: Logging infrastructure
  - HiLog integration when `CONFIG_HILOG` defined
  - Fallback to printf for non-HiLog builds
  - Parcel-specific debug logging support

- **flat_obj.h**: Binder IPC type definitions
  - Platform-sensitive types for 32/64-bit
  - `parcel_flat_binder_object` structure for IPC

- **unistd_ohos_mock.h**: Musl compatibility macros
  - Maps OHOS-specific process ID functions to POSIX

- **xxd.h**: Hex dump utility with color coding
  - Colorized hex dump similar to `xxd` command

## Flow

1. **Error Handling**: Subsystems define error bases using `ErrCodeOffset()`, then module-specific errors
2. **Thread-Safe Collections**: All operations use `std::lock_guard<std::mutex>`, some use condition variables
3. **Singleton Access**: `GetInstance()` returns singleton, `DestroyInstance()` for cleanup (delayed variants)
4. **Logging**: Macros expand to HiLog calls or printf depending on build configuration

## Integration

**Dependencies:**
- lib/nocopyable (NoCopyable base class)
- lib/musl_ohos_mock (for unistd compatibility)

**Used By:**
- Virtually all other libraries in the system
- lib/file_ex, lib/directory_ex, lib/unicode_ex, etc.

**CMake:** Interface library (`add_library(misc INTERFACE)`)
- Header-only library, no compilation required
