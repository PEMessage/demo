# lib/ashmem/

Anonymous Shared Memory (Ashmem) library for OpenHarmony. Provides a C++ wrapper around the Linux ashmem driver for efficient inter-process communication through shared memory.

## Responsibility

**Anonymous Shared Memory for IPC**

- Creates kernel-level anonymous memory regions that can be shared between processes
- Provides a file descriptor-based mechanism to pass shared memory across process boundaries
- Enables zero-copy data transfer between processes (producer/consumer pattern)
- Manages memory mapping/unmapping between kernel space and user space
- Enforces access control via protection flags (read/write)

**Key Use Cases:**
- Large data transfer between processes without copying
- Graphics buffer sharing (e.g., SurfaceFlinger)
- Multimedia data sharing
- High-performance IPC scenarios

## Design

### Core Abstractions

```
┌─────────────────────────────────────────────────────────────┐
│                        Ashmem Class                          │
├─────────────────────────────────────────────────────────────┤
│  memoryFd_     → File descriptor to /dev/ashmem              │
│  memorySize_   → Size of the shared memory region            │
│  flag_         → mmap protection flags (PROT_READ/WRITE)     │
│  startAddr_    → User-space pointer (mmap result)            │
└─────────────────────────────────────────────────────────────┘
                      │
                      ▼
            ┌─────────────────┐
            │  Linux Ashmem   │  (kernel driver)
            │    Driver       │
            └─────────────────┘
```

### Key Design Patterns

1. **Reference Counting**: Inherits from `RefBase`, managed via `sptr<Ashmem>` smart pointers
2. **RAII**: Destructor automatically unmaps and closes the ashmem region
3. **Thread Safety**: Global `pthread_mutex_t` protects `/dev/ashmem` device access during creation
4. **Dual API**: C-style functions (`AshmemCreate`, `AshmemSetProt`) + C++ class (`Ashmem`)
5. **Rust Compatibility**: Conditional `UTILS_CXX_RUST` flag enables Rust FFI integration

### Memory Protection Model

- **Kernel-level**: Set via `ASHMEM_SET_PROT_MASK` ioctl
- **User-level**: Enforced via `mmap()` protection flags
- **Access Validation**: `CheckValid()` verifies bounds and permissions before read/write

## Flow

### Creation Flow

```
Process A                                    Kernel
  │                                            │
  │  Ashmem::CreateAshmem("name", size)        │
  │────────────────────────────────────────────>│
  │                                            │
  │  1. AshmemOpen()                           │
  │     ├── open("/dev/ashmem", O_RDWR)        │──> Creates kernel ashmem region
  │     └── fstat() validation                 │
  │                                            │
  │  2. ASHMEM_SET_NAME ioctl                  │──> Sets region name (debugging)
  │                                            │
  │  3. ASHMEM_SET_SIZE ioctl                  │──> Sets region size
  │                                            │
  │<───────────────────────────────────────────│ Returns: file descriptor (fd)
  │                                            │
  │  4. new Ashmem(fd, size)                   │
  │     └── sptr<Ashmem> (RefBase)             │
```

### Mapping Flow

```
Ashmem::MapAshmem(PROT_READ | PROT_WRITE)
         │
         ▼
   ┌─────────────┐
   │  mmap()     │  ← Maps kernel ashmem to user space
   └─────────────┘
         │
         ▼
   startAddr_ = virtual address in process memory
   flag_      = protection mode
```

### Sharing Flow (Between Processes)

```
Process A (Creator)               Process B (Consumer)
     │                                    │
     │  1. Create Ashmem                  │
     │     Ashmem::CreateAshmem()         │
     │                                    │
     │  2. Map for writing                │
     │     MapReadAndWriteAshmem()        │
     │                                    │
     │  3. Write data                     │
     │     WriteToAshmem(data, size, 0)   │
     │                                    │
     │  4. Pass fd via IPC ───────────────>│  (e.g., via Binder/Parcel)
     │     (file descriptor transfer)      │
     │                                    │  5. Construct from fd
     │                                    │     Ashmem ashmem(fd, size)
     │                                    │
     │                                    │  6. Map for reading
     │                                    │     MapReadOnlyAshmem()
     │                                    │
     │                                    │  7. Read data
     │                                    │     ReadFromAshmem(size, 0)
```

### Cleanup Flow

```
~Ashmem() / CloseAshmem()
         │
    ┌────┴────┐
    ▼         ▼
 UnmapAshmem  close(memoryFd_)
    │              │
    ▼              ▼
 munmap()      Kernel releases
 (user space)  ashmem region
```

## Integration

### Dependencies

```cmake
# CMakeLists.txt
target_link_libraries(ashmem PUBLIC misc refbase parcel)
```

| Dependency | Purpose |
|------------|---------|
| `refbase`  | Reference counting base class (`RefBase`, `sptr`) |
| `parcel`   | IPC data marshalling (for fd passing via Binder) |
| `misc`     | Utility functions |
| `securec`  | Safe memory operations (`strcpy_s`, `memcpy_s`) |
| `utils_log`| Logging macros (`UTILS_LOGE`) |

### Linux Ashmem Driver Interface

```c
#include <linux/ashmem.h>
// Uses ioctls:
// - ASHMEM_SET_NAME
// - ASHMEM_SET_SIZE
// - ASHMEM_SET_PROT_MASK
// - ASHMEM_GET_SIZE
// - ASHMEM_GET_PROT_MASK
```

### Usage Example

```cpp
#include "ashmem.h"
using namespace OHOS;

// Create 4KB shared memory region
sptr<Ashmem> ashmem = Ashmem::CreateAshmem("MySharedMem", 4096);

// Map for read/write access
ashmem->MapReadAndWriteAshmem();

// Write data
const char* data = "Hello, Shared Memory!";
ashmem->WriteToAshmem(data, strlen(data), 0);

// Pass ashmem->GetAshmemFd() to another process via IPC
// ...

// Cleanup (automatic in destructor)
ashmem->UnmapAshmem();
ashmem->CloseAshmem();
```

### File Structure

```
lib/ashmem/
├── CMakeLists.txt    # Build configuration (static library)
├── ashmem.h          # Public API header
├── ashmem.cpp        # Implementation
└── codemap.md        # This documentation
```
