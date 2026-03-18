# IPC Demos - Inter-Process Communication Examples

This directory conceptually groups the IPC-related demos. These demos demonstrate OpenHarmony's IPC mechanisms from basic serialization to full RPC frameworks.

## Overview

| Demo | Complexity | Purpose |
|------|------------|---------|
| **parcel_demo** | Basic | Basic Parcel serialization primitives |
| **message_parcel_demo** | Intermediate | IPC-ready MessageParcel with tokens |
| **rpc_server_demo** | Advanced | Full RPC with stub/proxy/death recipient |
| **process_skeleton_demo** | Advanced | Process-level IPC object management |

---

## parcel_demo

### Responsibility
Demonstrates the Parcel class for serializing and deserializing data. Shows how primitives, strings, vectors, and custom Parcelable objects are written to and read from a Parcel buffer.

### Design Patterns
- **Parcelable Interface**: Custom objects implement `Marshalling()` and `Unmarshalling()`
- **Type-Safe Writing**: Template-based write methods for various types
- **Automatic Alignment**: Parcel handles memory alignment automatically

### Key Features
```cpp
// Write operations
parcel.WriteInt32(42);
parcel.WriteFloat(3.14f);
parcel.WriteString("Hello, Parcel!");
parcel.WriteInt32Vector({1, 2, 3, 4, 5});
parcel.WriteParcelable(&parcelable);

// Read operations
parcel.ReadInt32(value);
parcel.ReadFloat(floatValue);
std::string str = parcel.ReadString();
parcel.ReadInt32Vector(&vector);
MyParcelable *obj = MyParcelable::Unmarshalling(parcel);
```

### Visual Output
The demo uses `xxd_color2()` to display hex dumps of parcel data, showing:
- Data size (4 bytes)
- String data with padding for alignment
- Vector size prefix before elements

### Build & Run
```bash
cd parcel_demo
mkdir build && cd build
cmake .. && make
./app
```

---

## message_parcel_demo

### Responsibility
Extends Parcel with IPC-specific features. Demonstrates MessageParcel for inter-process communication, including interface tokens and exception handling.

### Key Features
```cpp
MessageParcel parcel;

// Write IPC data
parcel.WriteInt32(42);
parcel.WriteString16(u"Hello, IPC!");  // UTF-16 for IPC
parcel.WriteInterfaceToken(u"test.interface.token");
parcel.WriteNoException();

// Read IPC data
int32_t value = parcel.ReadInt32();
std::u16string token = parcel.ReadInterfaceToken();
int32_t exception = parcel.ReadException();
```

### IPC-Specific APIs
- `WriteString16()` / `ReadString16()`: UTF-16 strings for IPC compatibility
- `WriteInterfaceToken()`: Security token for interface validation
- `WriteNoException()`: Mark that no exception occurred
- `ReadException()`: Check for remote exceptions

### Dependencies
- **ipc**: MessageParcel implementation
- **ashmem**: Shared memory support (required by ipc)

### Build & Run
```bash
cd message_parcel_demo
mkdir build && cd build
cmake .. && make
./app
```

---

## rpc_server_demo

### Responsibility
Complete RPC demonstration with interface definition, stub (server-side), and proxy (client-side) implementations. Shows the full IPC broker pattern used in OpenHarmony.

### Architecture
```
IRemoteBroker (interface)
    └── IRpcFooTest (custom interface)
        ├── RpcFooStub (server implementation)
        └── RpcFooProxy (client proxy)
```

### Interface Definition (rpc_test.h)
```cpp
class IRpcFooTest : public IRemoteBroker {
public:
    enum FooInterFaceId {
        GET_FOO_NAME = 0,
        GET_TOKENID = 3,
        TEST_ADD = 5,
    };
    virtual std::string TestGetFooName(void) = 0;
    virtual int32_t TestAccessToken(MessageParcel &data, MessageParcel &reply) = 0;
    virtual int32_t TestAdd(MessageParcel &data, MessageParcel &reply) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"test.rpc.IRpcFooTest");
};
```

### Stub Implementation (RpcFooStub)
```cpp
class RpcFooStub : public IRemoteStub<IRpcFooTest> {
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                       MessageParcel &reply, MessageOption &option) override;
};
```

### Proxy Implementation (RpcFooProxy)
```cpp
class RpcFooProxy : public IRemoteProxy<IRpcFooTest> {
    std::string TestGetFooName(void) override;
    int32_t TestAdd(MessageParcel &data, MessageParcel &reply) override;
};
```

### Key RPC Operations
1. **SendRequest()**: Client sends RPC call to server
2. **OnRemoteRequest()**: Server dispatches to handler based on code
3. **Marshalling**: Data serialized to MessageParcel
4. **DeathRecipient**: Callback when remote process dies

### Build & Run
```bash
cd rpc_server_demo
mkdir build && cd build
cmake .. && make
./app  # Starts IPCSkeleton work thread
```

### Files
- `main.cpp`: Entry point, starts `IPCSkeleton::JoinWorkThread()`
- `rpc_test.h`: Interface, stub, proxy, and death recipient declarations
- `rpc_test.cpp`: Implementations

---

## process_skeleton_demo

### Responsibility
Demonstrates ProcessSkeleton - the singleton managing IPC objects within a process. Shows object attachment, querying, thread management, and invoker info tracking.

### Key APIs Demonstrated
```cpp
// Get singleton instance
ProcessSkeleton* skeleton = ProcessSkeleton::GetInstance();

// Object management
skeleton->AttachObject(object, descriptor, isLocal);
skeleton->QueryObject(descriptor, isLocal);
skeleton->IsContainsObject(object);
skeleton->DetachObject(object, descriptor);

// Registry access
sptr<IRemoteObject> registry = skeleton->GetRegistryObject();

// Invoker process info
skeleton->AttachInvokerProcInfo(isLocal, info);
skeleton->QueryInvokerProcInfo(isLocal, info);

// Thread management
skeleton->GetThreadStopFlag();
skeleton->SetIPCProxyLimit(limit, callback);
```

### Build & Run
```bash
cd process_skeleton_demo
mkdir build && cd build
cmake .. && make
./app
```

### Dependencies
- **ipc**: Core IPC functionality
- **ashmem**: Shared memory
- **misc**: Miscellaneous utilities

---

## Integration

### All IPC Demos Link To
- **ipc**: `/home/zhuojw/demo_new/c++/ohos/lib/ipc/`
- **ashmem**: `/home/zhuojw/demo_new/c++/ohos/lib/ashmem/` (required by ipc)

### CMake Pattern
```cmake
add_executable(app main.cpp [extra_sources])
auto_target_link_libraries(app PRIVATE ipc ashmem)
```

## Learning Path

1. **Start with parcel_demo**: Understand basic serialization
2. **Progress to message_parcel_demo**: Learn IPC-specific features
3. **Study rpc_server_demo**: See full RPC architecture
4. **Review process_skeleton_demo**: Understand process-level management

## Common IPC Types

| Type | Usage |
|------|-------|
| `MessageParcel` | Data container for IPC |
| `MessageOption` | IPC call options (sync/async) |
| `IRemoteObject` | Base for all IPC-capable objects |
| `IRemoteBroker` | Interface base class |
| `IRemoteStub` | Server-side implementation base |
| `IRemoteProxy` | Client-side proxy base |
| `sptr<>` | Smart pointer for reference counting |
