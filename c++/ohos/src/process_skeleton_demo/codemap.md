# process_skeleton_demo - Process Skeleton IPC Management

## Responsibility

Demonstrates `ProcessSkeleton` - the singleton managing IPC objects within a process. Shows object attachment/query, thread management, invoker process info tracking, and IPC proxy limits.

## Design

### ProcessSkeleton Singleton
```cpp
// Get the singleton instance
ProcessSkeleton* processSkeleton = ProcessSkeleton::GetInstance();
```

### Object Management
```cpp
// Attach object with descriptor
processSkeleton->AttachObject(object, descriptor, isLocal);

// Query object by descriptor
sptr<IRemoteObject> obj = processSkeleton->QueryObject(descriptor, isLocal);

// Check containment
bool contained = processSkeleton->IsContainsObject(object);

// Detach object
processSkeleton->DetachObject(object, descriptor);
```

### Invoker Process Info
```cpp
// Attach local/remote process info
InvokerProcInfo info = {pid, uid, tokenId, ...};
processSkeleton->AttachInvokerProcInfo(isLocal, info);

// Query process info
InvokerProcInfo queried;
processSkeleton->QueryInvokerProcInfo(isLocal, queried);
```

## Flow

```
main.cpp
├── Get ProcessSkeleton singleton instance
│
├── 1. Object management demo
│   ├── Create TestRemoteObject
│   ├── AttachObject(object, descriptor, true)
│   ├── QueryObject(descriptor, true)
│   └── Verify IsContainsObject()
│
├── 2. Registry object demo
│   └── GetRegistryObject() (may be null)
│
├── 3. Invoker process info demo
│   ├── AttachInvokerProcInfo(true, localInfo)
│   └── QueryInvokerProcInfo(true, queriedInfo)
│
├── 4. Thread management demo
│   └── GetThreadStopFlag()
│
├── 5. IPC proxy limit demo
│   ├── SetIPCProxyLimit(1000, callback)
│   └── Callback invoked when limit reached
│
├── Cleanup
│   ├── DetachObject()
│   └── DetachInvokerProcInfo()
│
└── Return success
```

## Integration

### Dependencies
- **ipc**: Core IPC framework
- **ashmem**: Shared memory
- **misc**: Miscellaneous utilities

### Files
- `server_main.cpp`: Demo entry point

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app server_main.cpp)
target_include_directories(app PUBLIC .)
auto_target_link_libraries(app PRIVATE ipc ashmem misc)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
ProcessSkeleton API Demo

1. Object management demo:
Object attached successfully
Object queried successfully
Object is contained in skeleton

2. Registry object demo:
No registry object set

3. Invoker process info demo:
Local invoker info attached
Queried local invoker PID: 123

4. Thread management demo:
Thread stop flag: 0

5. IPC proxy limit demo:
IPC proxy limit set to 1000

Demo completed successfully!
```

### Key APIs

| API | Purpose |
|-----|---------|
| `GetInstance()` | Get ProcessSkeleton singleton |
| `AttachObject(obj, desc, local)` | Register object |
| `QueryObject(desc, local)` | Lookup object by descriptor |
| `IsContainsObject(obj)` | Check if object registered |
| `DetachObject(obj, desc)` | Unregister object |
| `GetRegistryObject()` | Get registry service |
| `AttachInvokerProcInfo(local, info)` | Set process info |
| `QueryInvokerProcInfo(local, info)` | Get process info |
| `GetThreadStopFlag()` | Check thread state |
| `SetIPCProxyLimit(limit, cb)` | Set proxy limit callback |

### InvokerProcInfo Structure
```cpp
struct InvokerProcInfo {
    pid_t pid;           // Process ID
    pid_t uid;           // User ID
    uint32_t tokenId;    // Access token ID
    uint32_t deviceId;   // Device ID
    uint32_t networkId;  // Network ID
    std::string sid;     // Session ID
    int32_t status;      // Status
};
```

### TestRemoteObject Example
```cpp
class TestRemoteObject : public IRemoteObject {
public:
    TestRemoteObject(): IRemoteObject(u"test.descriptor") {}
    
    int SendRequest(uint32_t code, MessageParcel &data,
                   MessageParcel &reply, MessageOption &option) override {
        return 0;
    }
    // ... other required overrides
};
```

## Notes

- ProcessSkeleton is per-process singleton
- Objects are identified by UTF-16 descriptor strings
- Local vs remote flags distinguish same-process vs cross-process
- Invoker info tracks process details for security/audit
- IPC proxy limit helps prevent resource exhaustion
- Thread stop flag used for graceful shutdown
