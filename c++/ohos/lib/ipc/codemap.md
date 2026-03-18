# OpenHarmony IPC Library Codemap

## Module: lib/ipc

**Location**: `/home/zhuojw/demo_new/c++/ohos/lib/ipc`

---

## 1. Responsibility

### What This Module Does

The IPC (Inter-Process Communication) library is OpenHarmony's core framework for enabling communication between processes. It provides:

- **Synchronous and Asynchronous IPC**: Thread-safe message passing between processes
- **RPC (Remote Procedure Call)**: Cross-device communication via SoftBus/D-Bus
- **Object Marshalling/Unmarshalling**: Serialization of remote object references across process boundaries
- **Death Notification**: Callback mechanism when remote processes die
- **Thread Pool Management**: Automatic thread spawning for handling concurrent requests
- **Security Integration**: Token-based access control and caller identity verification

### Key Capabilities

| Feature | Description |
|---------|-------------|
| Binder IPC | Linux kernel binder driver integration for intra-device IPC |
| D-Bus/SoftBus | Distributed communication across devices |
| File Descriptor Passing | Sharing FDs between processes |
| Ashmem Support | Anonymous shared memory for large data transfers |
| Death Recipients | Notification when remote objects become unavailable |

---

## 2. Design

### Architecture Patterns

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           CLIENT APPLICATION                                │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐ │
│  │ IRemoteProxy    │  │ IRemoteObject   │  │ MessageParcel               │ │
│  │ (Client-side    │──│ (Base class)    │──│ (Data serialization)        │ │
│  │  interface)     │  │                 │  │                             │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────────┘ │
│           │                    │                       │                   │
│           ▼                    ▼                       ▼                   │
│  ┌──────────────────────────────────────────────────────────────────────┐ │
│  │                     IPCObjectProxy                                   │ │
│  │  - Holds handle to remote stub                                       │ │
│  │  - Manages death recipients                                          │ │
│  │  - Routes SendRequest() calls                                        │ │
│  └──────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
└────────────────────────────────────┼────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         INVOKER LAYER (Abstract)                           │
│  ┌─────────────────────────┐    ┌─────────────────────────────────────────┐ │
│  │    BinderInvoker        │    │    DBinderDatabusInvoker (RPC)          │ │
│  │  - Kernel binder ops    │    │  - SoftBus socket communication         │ │
│  │  - BR/BC commands       │    │  - Cross-device session management      │ │
│  │  - Thread loop handling │    │  - Session lifecycle management         │ │
│  └─────────────────────────┘    └─────────────────────────────────────────┘ │
│                                     ▲                                      │
│                         ┌───────────┴───────────┐                          │
│                         ▼                       ▼                          │
│              ┌─────────────────────┐  ┌─────────────────────┐              │
│              │ InvokerFactory      │  │ IRemoteInvoker      │              │
│              │ (Protocol registry) │  │ (Interface)         │              │
│              └─────────────────────┘  └─────────────────────┘              │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SERVER APPLICATION                                  │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                     IPCObjectStub                                    │  │
│  │  - Implements OnRemoteRequest() dispatcher                           │  │
│  │  - Standard transaction codes (PING, DUMP, INTERFACE)                │  │
│  │  - DBus extension support (session management)                       │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                    ▲                                        │
│           ┌────────────────────────┼────────────────────────┐               │
│           │                        │                        │               │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────────┐   │
│  │ IRemoteStub     │     │ MessageParcel   │     │ MessageOption       │   │
│  │ (Server-side    │     │ (Request/Reply  │     │ (Sync/Async flags)  │   │
│  │  implementation)│     │  data container)│     │                     │   │
│  └─────────────────┘     └─────────────────┘     └─────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Core Design Patterns

#### 1. Proxy-Stub Pattern

```
┌─────────────────┐                      ┌─────────────────┐
│   IRemoteProxy  │ ───────uses───────── │  IPCObjectProxy │
│   (Template)    │                      │  (Handle-based) │
└─────────────────┘                      └────────┬────────┘
       │                                          │
       │         INTERFACE                        │  SendRequest()
       │         (generated)                      │
       │                                          │
┌─────────────────┐                      ┌─────────────────┐
│   IRemoteStub   │ ───────implements─── │  IPCObjectStub  │
│   (Template)    │                      │  (Cookie-based) │
└─────────────────┘                      └────────┬────────┘
                                                  │
                                           OnRemoteRequest()
                                                  │
                                           ┌──────┴──────┐
                                           ▼             ▼
                                    ┌──────────┐   ┌──────────┐
                                    │  USER    │   │  SYSTEM  │
                                    │ HANDLER  │   │  CODES   │
                                    └──────────┘   └──────────┘
```

**Key Insight**: The proxy-stub pattern provides type-safe IPC where:
- **Proxy** (client-side): Implements the interface by forwarding calls via `SendRequest()`
- **Stub** (server-side): Receives calls via `OnRemoteRequest()` and dispatches to implementation

#### 2. Invoker Pattern

The invoker pattern abstracts the underlying communication mechanism:

```cpp
// From: ipc/native/src/core/invoker/include/iremote_invoker.h
class IRemoteInvoker {
    virtual int SendRequest(int handle, uint32_t code, MessageParcel &data, 
                           MessageParcel &reply, MessageOption &option) = 0;
    virtual bool FlattenObject(Parcel &parcel, const IRemoteObject *object) = 0;
    virtual sptr<IRemoteObject> UnflattenObject(Parcel &parcel) = 0;
    // ...
};
```

**Implementations**:
- **BinderInvoker**: Uses Linux kernel binder driver (`/dev/binder`)
- **DBinderDatabusInvoker**: Uses SoftBus sockets for cross-device RPC

#### 3. MessageParcel - Data Serialization

```cpp
// From: interfaces/innerkits/ipc_core/include/message_parcel.h
class MessageParcel : public Parcel {
    bool WriteRemoteObject(const sptr<IRemoteObject> &object);
    sptr<IRemoteObject> ReadRemoteObject();
    bool WriteFileDescriptor(int fd);
    int ReadFileDescriptor();
    bool WriteInterfaceToken(std::u16string name);
    bool WriteRawData(const void *data, size_t size);
    // ...
};
```

**MessageParcel** extends the base `Parcel` class with IPC-specific capabilities:
- Remote object marshalling/unmarshalling
- File descriptor passing via `SCM_RIGHTS`
- Interface token for security verification
- Raw data support for large payloads (via ashmem)

### Key Data Structures

#### flat_binder_object (Kernel Interface)
```cpp
// Object representation passed to/from kernel binder driver
struct flat_binder_object {
    struct binder_object_header hdr;  // BINDER_TYPE_BINDER/HANDLE/FD
    __u32 flags;
    union {
        binder_uintptr_t binder;      // Local object address (stub)
        __u32 handle;                 // Remote reference index (proxy)
    };
    binder_uintptr_t cookie;          // Protocol info (IF_PROT_BINDER/DATABUS)
};
```

#### binder_transaction_data
```cpp
// Transaction structure for SendRequest/Reply
struct binder_transaction_data {
    union {
        __u32 handle;                 // Target handle (proxy)
        binder_uintptr_t ptr;         // Target object (stub)
    } target;
    __u32 code;                       // Operation code
    __u32 flags;                      // TF_SYNC, TF_ASYNC, TF_ACCEPT_FDS
    struct binder_buffer_object {     // Data buffer
        binder_uintptr_t buffer;
        binder_size_t offsets;
    } data;
    // ...
};
```

---

## 3. Flow

### IPC Call Flow (Intra-Device)

```
Client Side                                    Kernel                               Server Side
───────────                                   ──────                              ───────────
     │                                           │                                       │
     │  1. IRemoteProxy::Method()                │                                       │
     │     └─> AsObject()->SendRequest()         │                                       │
     │                                           │                                       │
     ▼                                           │                                       │
┌────────────┐                                   │                                       │
│IPCObjectProxy                                  │                                       │
│SendRequest()│                                  │                                       │
└─────┬──────┘                                   │                                       │
      │                                          │                                       │
      │  2. Get invoker (BinderInvoker)          │                                       │
      │     └─> invoker->SendRequest()           │                                       │
      │                                          │                                       │
      ▼                                          │                                       │
┌────────────┐                                   │                                       │
│BinderInvoker                                  │                                       │
│WriteTransaction()                             │                                       │
│  BC_TRANSACTION                               │                                       │
└─────┬──────┘                                   │                                       │
      │                                          │                                       │
      │  3. ioctl(BINDER_WRITE_READ)             │                                       │
      │──────────────────────────────────────────>                                       │
      │                                          │                                       │
      │                                          │  4. Find target stub                  │
      │                                          │     from object_map                   │
      │                                          │                                       │
      │                                          │  5. Copy transaction data             │
      │                                          │     to server process                 │
      │                                          │                                       │
      │                                          │──────────────────────>                 │
      │                                          │                      │                 │
      │                                          │  6. Wake server      │                 │
      │                                          │                      │                 │
      │                                          │<──────────────────────                 │
      │                                          │                      ▼                 │
      │                                          │               ┌────────────┐           │
      │                                          │               │IPCObjectStub│          │
      │                                          │               │SendRequest()│          │
      │                                          │               └─────┬──────┘           │
      │                                          │                     │                  │
      │                                          │              7. OnRemoteRequest()      │
      │                                          │                     │                  │
      │                                          │                     ▼                  │
      │                                          │              ┌────────────┐            │
      │                                          │              │IRemoteStub │            │
      │                                          │              │Dispatch to │            │
      │                                          │              │Impl        │            │
      │                                          │              └────────────┘            │
      │                                          │                                       │
      │  8. WaitForCompletion()                  │                                       │
      │<──────────────────────────────────────────│                                       │
      │  BR_REPLY or BR_TRANSACTION_COMPLETE      │                                       │
      │                                          │                                       │
      ▼                                          │                                       │
┌────────────┐                                   │                                       │
│HandleReply │                                   │                                       │
│Return result│                                  │                                       │
└────────────┘                                   │                                       │
```

### Cross-Device RPC Flow (DBinder/SoftBus)

```
Device A (Client)                                          Device B (Server)
─────────────────                                          ─────────────────

┌─────────────────┐                                         ┌─────────────────┐
│ IPCObjectProxy  │                                         │ IPCObjectStub   │
│ (IF_PROT_DATABUS)│                                        │ (via DBinder)   │
└────────┬────────┘                                         └─────────────────┘
         │
         │  1. First call triggers session setup
         │     └─> GetSessionName() from samgr
         │
         ▼
┌─────────────────┐
│DBinderDatabusInvoker
│CreateClientSocket()
│(SoftBus API)
└────────┬────────┘
         │
         │  2. SoftBus establishes P2P connection
         │     └─> DBinderGrantPermission()
         │
         │──────────────────────────────────────────────────────────────────────>
         │                                                      ┌──────────────┐
         │                                                      │DatabusSocketListener
         │                                                      │OnMessageAvailable
         │                                                      └──────┬───────┘
         │                                                             │
         │                                                      3. Session authentication
         │                                                      4. OnTransaction()
         │                                                             │
         │                                                      ┌──────┴───────┐
         │                                                      │DBinderDatabusInvoker
         │                                                      └──────────────┘
         │<──────────────────────────────────────────────────────────────────────
         │
         │  5. Send dbinder_transaction_data
         │     with stub_index for dispatch
         │
         │──────────────────────────────────────────────────────────────────────>
         │                                                             │
         │                                                      6. QueryStubByIndex()
         │                                                      7. Dispatch to stub
         │                                                             │
         │<──────────────────────────────────────────────────────────────
         │  8. Reply via same socket
         ▼
```

### Object Lifecycle Flow

```
PROXY LIFECYCLE
───────────────

1. CREATION
   IPCProcessSkeleton::FindOrNewObject()
        │
        ▼
   ┌───────────────┐
   │  new IPCObjectProxy(handle)
   │  - descriptor from handle
   │  - AcquireHandle() on binder
   └───────┬───────┘
           │
           ▼
   WaitForInit() ──> UpdateProto() ──> [IF_PROT_BINDER or IF_PROT_DATABUS]

2. USAGE
   SendRequest() ──> WriteRemoteObject() ──> FlattenObject()
        │
        └─> Binder: flat_binder_object{BINDER_TYPE_HANDLE, handle}
        └─> DBinder: dbinder_negotiation_data{stub_index, device_id, session_name}

3. DESTRUCTION
   OnLastStrongRef()
        │
        ▼
   ReleaseHandle(handle) ──> BC_RELEASE to kernel
   DetachObject(this) ──> Remove from object_map


STUB LIFECYCLE
───────────────

1. REGISTRATION
   IRemoteStub constructed
        │
        ▼
   IPCObjectStub ctor ──> AttachObject() to skeleton
        │
        └─> OnFirstStrongRef()
            └─> AttachValidObject() for integrity checking

2. REFERENCE COUNTING
   Kernel sends BR_ACQUIRE / BR_RELEASE
        │
        ▼
   BinderInvoker::OnAcquireObject() / OnReleaseObject()
        │
        └─> IncStrongRef() / DecStrongRef()

3. DEATH HANDLING (Proxy side)
   Remote process dies
        │
        ▼
   Kernel sends BR_DEAD_BINDER
        │
        ▼
   BinderInvoker::OnBinderDied()
        │
        └─> IPCObjectProxy::SendObituary()
            └─> Call all DeathRecipient::OnRemoteDied()
```

---

## 4. Integration

### Dependencies

```
┌────────────────────────────────────────────────────────────┐
│                     lib/ipc                                │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       │
│  │  Core IPC    │ │   Invoker    │ │   DBinder    │       │
│  │  Framework   │ │   Factory    │ │   (RPC)      │       │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘       │
└─────────┼────────────────┼────────────────┼────────────────┘
          │                │                │
          ▼                ▼                ▼
┌────────────────────────────────────────────────────────────┐
│  lib/utils ──> Parcel, String utilities                    │
│  lib/hilog ──> Logging infrastructure                      │
│  lib/ashmem ─> Anonymous shared memory                     │
│  lib/securec ─> Safe string operations                     │
│  kernel/binder ─> Binder driver (/dev/binder)             │
│  lib/softbus ──> Distributed communication (optional RPC) │
│  lib/access_token ─> Permission/token verification         │
│  lib/hitrace ──> Performance tracing                       │
└────────────────────────────────────────────────────────────┘
```

### How Other Modules Use IPC

#### Service Registration (System Ability Manager)

```cpp
// Service side - Register a system ability
sptr<IRemoteObject> service = new MyServiceStub();
IPCSkeleton::SetContextObject(service);  // Set as context manager
// or
samgr->AddSystemAbility(id, service);    // Register with SAM

// Client side - Get proxy to service
sptr<IRemoteObject> object = samgr->GetSystemAbility(id);
sptr<IMyService> proxy = iface_cast<IMyService>(object);
proxy->MyMethod(data);  // IPC call
```

#### Interface Definition (IDL Generated)

```cpp
// MyService interface definition (auto-generated)
class IMyService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.my.IMyService")
    virtual int MyMethod(const Data& data) = 0;
};

// Proxy implementation
class MyServiceProxy : public IRemoteProxy<IMyService> {
public:
    int MyMethod(const Data& data) override {
        MessageParcel parcel;
        parcel.WriteInterfaceToken(GetDescriptor());
        data.Marshalling(parcel);
        MessageParcel reply;
        MessageOption option;
        sptr<IRemoteObject> remote = Remote();
        return remote->SendRequest(MY_METHOD_CODE, parcel, reply, option);
    }
};

// Stub implementation  
class MyServiceStub : public IRemoteStub<IMyService> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel& data, 
                        MessageParcel& reply, MessageOption& option) override {
        if (code == MY_METHOD_CODE) {
            Data d;
            d.Unmarshalling(data);
            int result = MyMethod(d);
            reply.WriteInt32(result);
            return ERR_NONE;
        }
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
};
```

### Threading Model

```
┌─────────────────────────────────────────────────────────────────┐
│                    IPC THREAD ARCHITECTURE                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │  Main Thread │    │ IPC Work     │    │ IPC Work     │      │
│  │              │    │ Thread 1     │    │ Thread N     │      │
│  │              │    │              │    │              │      │
│  │ IPCThreadSkeleton           JoinThread() / StartWorkLoop()   │
│  │  │           │    │  │           │    │  │           │       │
│  │  ▼           │    │  ▼           │    │  ▼           │       │
│  │ BinderInvoker│    │ BinderInvoker│    │ BinderInvoker│       │
│  │ (default)    │    │ (registered) │    │ (registered) │       │
│  └──────────────┘    └──────────────┘    └──────────────┘       │
│         │                   │                   │              │
│         └───────────────────┼───────────────────┘              │
│                             │                                   │
│                             ▼                                   │
│                   ┌────────────────────┐                       │
│                   │  BinderConnector    │                       │
│                   │  (singleton)        │                       │
│                   │  /dev/binder        │                       │
│                   └────────────────────┘                       │
│                             │                                   │
└─────────────────────────────┼───────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  Kernel Binder  │
                    │  Driver         │
                    └─────────────────┘

Thread Spawn Conditions:
- BR_SPAWN_LOOPER received from kernel
- Explicit SpawnThread() call
- Thread pool size configurable via SetMaxWorkThreadNum()
```

### Configuration Options

| Macro | Description | Files Affected |
|-------|-------------|----------------|
| `CONFIG_IPC_SINGLE` | Single-device IPC only (no RPC) | All `#ifndef CONFIG_IPC_SINGLE` blocks |
| `ENABLE_IPC_TRACE` | Performance tracing support | ipc_trace.cpp, binder_invoker.cpp |
| `FFRT_IPC_ENABLE` | FFRT (Fast Function Runtime) integration | binder_invoker.cpp |
| `WITH_SELINUX` | SELinux context support | binder_invoker.cpp |
| `OHOS_PLATFORM` | Platform-specific features | ipc_object_proxy.cpp |

---

## 5. File Organization

```
lib/ipc/
├── CMakeLists.txt                    # Build configuration
│
├── interfaces/
│   └── innerkits/
│       └── ipc_core/
│           └── include/
│               ├── iremote_broker.h       # Base interface marker
│               ├── iremote_object.h       # Remote object base
│               ├── iremote_proxy.h        # Proxy template
│               ├── iremote_stub.h         # Stub template
│               ├── ipc_object_proxy.h     # Proxy implementation
│               ├── ipc_object_stub.h      # Stub implementation
│               ├── message_parcel.h       # IPC data container
│               ├── message_option.h       # Transaction options
│               ├── ipc_skeleton.h         # Process-level IPC API
│               └── ipc_types.h            # Error codes and constants
│
├── ipc/native/src/core/
│   ├── framework/
│   │   ├── source/
│   │   │   ├── message_parcel.cpp       # Parcel serialization
│   │   │   ├── ipc_object_proxy.cpp     # Proxy implementation
│   │   │   ├── ipc_object_stub.cpp      # Stub implementation
│   │   │   ├── process_skeleton.cpp     # Object registry
│   │   │   ├── ipc_process_skeleton.cpp # Extended process state
│   │   │   ├── ipc_skeleton.cpp         # Public API implementation
│   │   │   ├── ipc_thread_skeleton.cpp  # Thread-local invoker mgmt
│   │   │   ├── ipc_thread_pool.cpp      # Worker thread management
│   │   │   └── iremote_object.cpp       # Base object methods
│   │   └── include/                     # Private headers
│   │
│   ├── invoker/
│   │   ├── source/
│   │   │   ├── binder_invoker.cpp       # Kernel binder invoker
│   │   │   ├── binder_connector.cpp     # /dev/binder interface
│   │   │   ├── invoker_factory.cpp      # Protocol registration
│   │   │   ├── invoker_rawdata.cpp      # Raw data handling
│   │   │   └── hitrace_invoker.cpp      # Tracing instrumentation
│   │   └── include/
│   │       ├── iremote_invoker.h        # Invoker interface
│   │       └── binder_invoker.h         # Binder implementation
│   │
│   └── dbinder/
│       ├── source/
│       │   ├── dbinder_databus_invoker.cpp  # RPC invoker
│       │   ├── dbinder_session_object.cpp   # Session state
│       │   ├── dbinder_callback_stub.cpp    # Death callbacks
│       │   ├── dbinder_softbus_client.cpp   # SoftBus wrapper
│       │   └── databus_socket_listener.cpp  # Socket events
│       └── include/                         # DBinder headers
│
└── utils/                           # Support utilities
    └── include/
        └── sys_binder.h             # Kernel binder structs
```

---

## 6. Key Classes Reference

| Class | Responsibility | Key Methods |
|-------|--------------|-------------|
| **IRemoteObject** | Base for all IPC-capable objects | `SendRequest()`, `IsProxyObject()`, `AddDeathRecipient()` |
| **IPCObjectProxy** | Client-side handle to remote | `SendRequestInner()`, `WaitForInit()`, `SendObituary()` |
| **IPCObjectStub** | Server-side request dispatcher | `SendRequest()`, `OnRemoteRequest()` |
| **IRemoteProxy<T>** | Template for client interfaces | `AsObject()`, `Remote()` |
| **IRemoteStub<T>** | Template for server implementations | `AsObject()`, `AsInterface()` |
| **MessageParcel** | IPC data serialization | `WriteRemoteObject()`, `ReadRemoteObject()`, `WriteFileDescriptor()` |
| **IRemoteInvoker** | Abstract transport interface | `SendRequest()`, `FlattenObject()`, `JoinThread()` |
| **BinderInvoker** | Kernel binder implementation | `TransactWithDriver()`, `HandleCommands()`, `JoinThread()` |
| **DBinderDatabusInvoker** | RPC over SoftBus | `OnMessageAvailable()`, `SendData()`, `UpdateClientSession()` |
| **IPCProcessSkeleton** | Per-process object registry | `QueryObject()`, `AttachObject()`, `SpawnThread()` |
| **IPCThreadSkeleton** | Thread-local invoker management | `GetDefaultInvoker()`, `GetRemoteInvoker()` |

---

## 7. Transaction Codes

### Standard Codes (from ipc_types.h)

| Code | Value | Purpose |
|------|-------|---------|
| `PING_TRANSACTION` | '_PNG' | Check if remote is alive |
| `DUMP_TRANSACTION` | '_DMP' | Request debug dump |
| `INTERFACE_TRANSACTION` | '_NTF' | Get interface descriptor |
| `SYNCHRONIZE_REFERENCE` | '_SYC' | Synchronize reference counts |
| `GET_PROTO_INFO` | '_GRI' | Get protocol information (for RPC) |
| `GET_SESSION_NAME` | '_GSN' | Get SoftBus session name |
| `DBINDER_INCREFS_TRANSACTION` | '_DIT' | Increment distributed ref |
| `DBINDER_DECREFS_TRANSACTION` | '_DDT' | Decrement distributed ref |
| `DBINDER_ADD_COMMAUTH` | '_DAC' | Add communication auth |
| `DBINDER_OBITUARY_TRANSACTION` | '_DOT' | Death notification |

---

## 8. Security Considerations

### Identity Verification

```cpp
// Caller identity available in any IPC handler
pid_t pid = IPCSkeleton::GetCallingPid();
pid_t uid = IPCSkeleton::GetCallingUid();
uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
std::string deviceId = IPCSkeleton::GetCallingDeviceID();
bool isLocal = IPCSkeleton::IsLocalCalling();
```

### Access Token Integration

- **IF_PROT_DATABUS** (RPC) connections require token verification
- `DBINDER_ADD_COMMAUTH` transaction validates peer identity
- `DBinderGrantPermission()` establishes session permissions

### SELinux Integration (Optional)

- Security context passed via `BR_TRANSACTION_SEC_CTX`
- `FLAT_BINDER_FLAG_TXN_SECURITY_CTX` enables context passing
- Accessible via `IPCSkeleton::GetCallingSid()`

---

*This codemap was generated by analyzing the OpenHarmony IPC library source code.*
