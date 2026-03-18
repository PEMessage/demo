# ashmem_demo - Anonymous Shared Memory Demo

## Responsibility

Demonstrates the OpenHarmony Ashmem (Anonymous Shared Memory) API for creating and managing memory regions shared between processes. Shows creation, mapping, protection settings, read/write operations, and cleanup.

## Design

### Ashmem Lifecycle
```cpp
// 1. Create Ashmem region
sptr<Ashmem> ashmem = Ashmem::CreateAshmem(name, size);

// 2. Set memory protection
ashmem->SetProtection(PROT_READ | PROT_WRITE);

// 3. Map into process address space
ashmem->MapReadAndWriteAshmem();

// 4. Read/Write operations
ashmem->WriteToAshmem(data, dataSize, offset);
const void* readData = ashmem->ReadFromAshmem(dataSize, offset);

// 5. Cleanup
ashmem->UnmapAshmem();
ashmem->CloseAshmem();
```

## Flow

```
main.cpp
├── Create Ashmem region (4KB, named "DemoAshmem")
│   └── Get file descriptor and size
├── Set protection (PROT_READ | PROT_WRITE)
├── Map for read-write access
│   └── Memory mapped into process space
├── Write "Hello, Ashmem!" to offset 0
├── Read data back from offset 0
│   └── Verify: "Hello, Ashmem!"
├── Unmap memory
├── Close Ashmem
└── Return success
```

## Integration

### Dependencies
- **ashmem**: `/home/zhuojw/demo_new/c++/ohos/lib/ashmem/`

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE ashmem)
```

### System Requirements
**Important:** Before running, ensure `/dev/ashmem` is accessible:
```bash
chmod 0666 /dev/ashmem
```

This is noted in the CMakeLists.txt as well:
```cmake
message(STATUS "NOTE: ------ `chmod 0666 /dev/ashmem` before run -----")
```

## Usage

### Running the Demo
```bash
chmod 0666 /dev/ashmem  # One-time setup
./app
```

**Expected Output:**
```
Ashmem created successfully. FD: 3, Size: 4096
Read from ashmem: Hello, Ashmem!
Ashmem demo completed successfully
```

### Key APIs

| API | Purpose |
|-----|---------|
| `Ashmem::CreateAshmem(name, size)` | Create named shared memory region |
| `GetAshmemFd()` | Get file descriptor |
| `GetAshmemSize()` | Get region size |
| `SetProtection(prot)` | Set memory protection flags |
| `MapReadAndWriteAshmem()` | Map with RW access |
| `MapReadOnlyAshmem()` | Map read-only |
| `WriteToAshmem(data, size, offset)` | Write data at offset |
| `ReadFromAshmem(size, offset)` | Read data from offset |
| `UnmapAshmem()` | Unmap from address space |
| `CloseAshmem()` | Release the ashmem resource |

### Protection Flags
- `PROT_READ`: Read access
- `PROT_WRITE`: Write access
- `PROT_EXEC`: Execute access (not used in demo)

## Notes

- Ashmem is Android/OpenHarmony's anonymous shared memory mechanism
- File descriptor can be passed between processes via IPC
- Memory is reference counted; closes when last reference dropped
- The demo uses `sptr<>` for automatic resource management
- Always unmap before closing to prevent memory leaks
