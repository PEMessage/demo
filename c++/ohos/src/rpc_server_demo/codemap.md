# rpc_server_demo - RPC Framework Demo

## Responsibility

Complete RPC demonstration with full broker pattern implementation. Shows interface definition, server-side stub, client-side proxy, and death recipient for handling remote process termination.

## Design

### Broker Pattern
```
IRemoteBroker (base)
    └── IRpcFooTest (interface)
            ├── RpcFooStub (server implementation)
            └── RpcFooProxy (client proxy)
```

### Interface Definition
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

### Stub (Server Side)
```cpp
class RpcFooStub : public IRemoteStub<IRpcFooTest> {
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                       MessageParcel &reply, MessageOption &option) override;
    // Implementation of interface methods
};

int RpcFooStub::OnRemoteRequest(uint32_t code, ...) {
    switch (code) {
        case GET_FOO_NAME:
            reply.WriteString(TestGetFooName());
            break;
        // ... other cases
    }
}
```

### Proxy (Client Side)
```cpp
class RpcFooProxy : public IRemoteProxy<IRpcFooTest> {
    std::string TestGetFooName(void) override {
        MessageOption option;
        MessageParcel data, reply;
        Remote()->SendRequest(GET_FOO_NAME, data, reply, option);
        return reply.ReadString();
    }
};
```

## Flow

```
main.cpp
└── IPCSkeleton::JoinWorkThread()
    └── Enters IPC event loop to process remote requests

rpc_test.cpp (Stub)
├── OnRemoteRequest(code, data, reply, option)
│   ├── GET_FOO_NAME: reply.WriteString(GetFooName())
│   ├── GET_TOKENID: TestAccessToken(data, reply)
│   ├── TEST_REMOTE_OBJECT: Pass remote object
│   └── TEST_ADD: Read a,b from data, write a+b to reply

rpc_test.cpp (Proxy)
├── TestGetFooName()
│   └── SendRequest(GET_FOO_NAME) → return reply.ReadString()
├── TestAccessToken(data, reply)
│   └── SendRequest(GET_TOKENID)
└── TestAdd(data, reply)
    └── SendRequest(TEST_ADD, data with a=223, b=513)
```

## Integration

### Dependencies
- **ipc**: Core IPC framework
- **ashmem**: Shared memory

### Files
- `main.cpp`: Entry point, starts IPCSkeleton
- `rpc_test.h`: Interface, stub, proxy declarations
- `rpc_test.cpp`: All implementations

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp rpc_test.cpp)
target_include_directories(app PRIVATE .)
auto_target_link_libraries(app PRIVATE ipc ashmem)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
[Enters IPC work thread - waits for remote requests]
```

### Key APIs

| API | Purpose |
|-----|---------|
| `DECLARE_INTERFACE_DESCRIPTOR` | Define interface UUID |
| `IRemoteStub<T>` | Base for server implementation |
| `IRemoteProxy<T>` | Base for client proxy |
| `OnRemoteRequest()` | Server dispatch method |
| `Remote()->SendRequest()` | Client RPC call |
| `BrokerDelegator` | Factory for proxy creation |
| `DeathRecipient` | Callback on remote death |

### RPC Call Sequence

```cpp
// Client side
RpcFooProxy proxy(remoteObject);
std::string name = proxy.TestGetFooName();

// Internally:
// 1. Create MessageParcel data, reply
// 2. SendRequest(GET_FOO_NAME, data, reply, option)
// 3. IPC marshals data, sends to server
// 4. Server OnRemoteRequest() dispatched
// 5. Server writes reply
// 6. Reply returned to client
// 7. Client unmarshals reply.ReadString()
```

### Death Recipient
```cpp
class RpcDeathRecipient : public IRemoteObject::DeathRecipient {
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override;
};

// Usage:
sptr<DeathRecipient> recipient = new RpcDeathRecipient();
remoteObject->AddDeathRecipient(recipient);
```

## Notes

- Full RPC framework with type-safe interface
- Interface codes (GET_FOO_NAME, etc.) must match between stub/proxy
- `SendRequest()` blocks until server responds (sync mode)
- Use `MessageOption` for sync/async configuration
- Death recipients notify when remote process terminates
- `iface_cast<T>()` casts IRemoteObject to specific proxy type
