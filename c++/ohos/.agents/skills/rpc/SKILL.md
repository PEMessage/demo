# Skill: OpenHarmony IPC/RPC Implementation

This skill provides templates and patterns for implementing IPC/RPC in OpenHarmony style.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Core Concepts](#core-concepts)
3. [Interface Definition](#interface-definition)
4. [Stub Implementation](#stub-implementation)
5. [Proxy Implementation](#proxy-implementation)
6. [Service Registration](#service-registration)
7. [Client Usage](#client-usage)
8. [Callback Pattern](#callback-pattern)
9. [Common Patterns](#common-patterns)

---

## Quick Start

### Minimal RPC Service

**Step 1: Define Interface** (`i_my_service.h`)
```cpp
#ifndef I_MY_SERVICE_H
#define I_MY_SERVICE_H

#include "iremote_broker.h"

namespace OHOS {

class IMyService : public IRemoteBroker {
public:
    enum {
        ADD = 1,
        GET_NAME = 2,
    };
    
    virtual int32_t Add(int32_t a, int32_t b) = 0;
    virtual std::string GetName() = 0;
    
    DECLARE_INTERFACE_DESCRIPTOR(u"my.service.IMyService");
};

} // namespace OHOS
#endif
```

**Step 2: Implement Stub** (`my_service_stub.h/.cpp`)
```cpp
// my_service_stub.h
#include "i_my_service.h"
#include "iremote_stub.h"

class MyServiceStub : public IRemoteStub<IMyService> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override;
    int32_t Add(int32_t a, int32_t b) override;
    std::string GetName() override;
};

// my_service_stub.cpp
int MyServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data,
                                    MessageParcel& reply, MessageOption& option) {
    switch (code) {
        case ADD: {
            int32_t a = data.ReadInt32();
            int32_t b = data.ReadInt32();
            int32_t result = Add(a, b);
            reply.WriteInt32(result);
            return ERR_NONE;
        }
        case GET_NAME: {
            std::string name = GetName();
            reply.WriteString(name);
            return ERR_NONE;
        }
        default:
            return IRemoteStub::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t MyServiceStub::Add(int32_t a, int32_t b) {
    return a + b;
}

std::string MyServiceStub::GetName() {
    return "MyService";
}
```

**Step 3: Implement Proxy** (`my_service_proxy.h/.cpp`)
```cpp
// my_service_proxy.h
#include "i_my_service.h"
#include "iremote_proxy.h"

class MyServiceProxy : public IRemoteProxy<IMyService> {
public:
    explicit MyServiceProxy(const sptr<IRemoteObject>& impl)
        : IRemoteProxy<IMyService>(impl) {}
    
    int32_t Add(int32_t a, int32_t b) override;
    std::string GetName() override;
};

// my_service_proxy.cpp
int32_t MyServiceProxy::Add(int32_t a, int32_t b) {
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    
    data.WriteInterfaceToken(GetDescriptor());
    data.WriteInt32(a);
    data.WriteInt32(b);
    
    int32_t result = Remote()->SendRequest(ADD, data, reply, option);
    if (result == ERR_NONE) {
        return reply.ReadInt32();
    }
    return result;
}

std::string MyServiceProxy::GetName() {
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    
    data.WriteInterfaceToken(GetDescriptor());
    
    int32_t result = Remote()->SendRequest(GET_NAME, data, reply, option);
    if (result == ERR_NONE) {
        return reply.ReadString();
    }
    return "";
}
```

**Step 4: Server Main** (`server/main.cpp`)
```cpp
#include "my_service_stub.h"
#include "samgr_mini.h"
#include "ipc_skeleton.h"

int main() {
    // Create service instance
    sptr<MyServiceStub> service = new MyServiceStub();
    
    // Register to Service Manager
    auto samgr = SamgrMini::GetInstance();
    samgr->AddService(u"my.service", service->AsObject());
    
    // Start IPC loop
    IPCSkeleton::JoinWorkThread();
    return 0;
}
```

**Step 5: Client Main** (`client/main.cpp`)
```cpp
#include "my_service_proxy.h"
#include "samgr_mini.h"
#include "ipc_skeleton.h"

int main() {
    // Get Service Manager
    sptr<IRemoteObject> samgrObj = IPCSkeleton::GetContextObject();
    sptr<IServiceRegistry> samgr = iface_cast<IServiceRegistry>(samgrObj);
    
    // Find service
    sptr<IRemoteObject> obj = samgr->CheckService(u"my.service");
    sptr<IMyService> service = iface_cast<IMyService>(obj);
    
    // Call methods
    int result = service->Add(10, 20);
    std::string name = service->GetName();
    
    return 0;
}
```

---

## Core Concepts

### Class Hierarchy

```
IRemoteBroker (interface marker)
    ↑
IRemoteObject (base IPC object)
    ↑
    ├── IRemoteStub<T> (server-side implementation)
    └── IRemoteProxy<T> (client-side proxy)
```

### Key Components

| Component | Role | Usage |
|-----------|------|-------|
| `IRemoteBroker` | Interface marker | Base for all IPC interfaces |
| `IRemoteObject` | Base class | Reference counted IPC object |
| `IRemoteStub` | Server implementation | Receives and handles IPC calls |
| `IRemoteProxy` | Client proxy | Sends IPC requests to remote |
| `IPCSkeleton` | Process-level IPC | Get context, join work thread |
| `MessageParcel` | Data container | Serialize/deserialize data |

---

## Interface Definition

### Basic Interface
```cpp
class ICalculator : public IRemoteBroker {
public:
    enum {
        ADD = 1,
        SUBTRACT = 2,
        MULTIPLY = 3,
        DIVIDE = 4,
    };
    
    virtual int32_t Add(int32_t a, int32_t b) = 0;
    virtual int32_t Subtract(int32_t a, int32_t b) = 0;
    virtual int32_t Multiply(int32_t a, int32_t b) = 0;
    virtual int32_t Divide(int32_t a, int32_t b) = 0;
    
    DECLARE_INTERFACE_DESCRIPTOR(u"example.ICalculator");
};
```

### Interface with Complex Types
```cpp
class IDataManager : public IRemoteBroker {
public:
    enum {
        SET_DATA = 1,
        GET_DATA = 2,
        NOTIFY_CHANGE = 3,
    };
    
    struct DataItem : public Parcelable {
        int32_t id;
        std::string name;
        std::vector<uint8_t> payload;
        
        bool Marshalling(MessageParcel& parcel) const override {
            parcel.WriteInt32(id);
            parcel.WriteString(name);
            parcel.WriteUInt8Vector(payload);
            return true;
        }
        
        bool Unmarshalling(MessageParcel& parcel) {
            id = parcel.ReadInt32();
            name = parcel.ReadString();
            parcel.ReadUInt8Vector(&payload);
            return true;
        }
    };
    
    virtual int32_t SetData(const DataItem& item) = 0;
    virtual int32_t GetData(int32_t id, DataItem& item) = 0;
    virtual int32_t RegisterCallback(const sptr<IRemoteObject>& callback) = 0;
    
    DECLARE_INTERFACE_DESCRIPTOR(u"example.IDataManager");
};
```

---

## Stub Implementation

### Basic Stub
```cpp
class CalculatorStub : public IRemoteStub<ICalculator> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override {
        // Verify interface token
        if (!CheckInterfaceToken(data)) {
            return ERR_INVALID_DATA;
        }
        
        switch (code) {
            case ADD: {
                int32_t a = data.ReadInt32();
                int32_t b = data.ReadInt32();
                int32_t result = Add(a, b);
                reply.WriteInt32(result);
                return ERR_NONE;
            }
            // ... other cases
            default:
                return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
    
    int32_t Add(int32_t a, int32_t b) override {
        return a + b;
    }
    
    // ... other implementations
};
```

### Thread-Safe Stub
```cpp
class ThreadSafeServiceStub : public IRemoteStub<IService> {
public:
    int32_t ProcessRequest(int32_t requestId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        // Process request
        return ProcessInternal(requestId);
    }
    
private:
    std::mutex mutex_;
};
```

---

## Proxy Implementation

### Synchronous Call
```cpp
int32_t CalculatorProxy::Add(int32_t a, int32_t b) {
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);  // Synchronous
    
    // Write interface token
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_DATA;
    }
    
    // Write parameters
    if (!data.WriteInt32(a) || !data.WriteInt32(b)) {
        return ERR_INVALID_DATA;
    }
    
    // Send request
    int32_t result = Remote()->SendRequest(ADD, data, reply, option);
    if (result != ERR_NONE) {
        return result;
    }
    
    // Read result
    return reply.ReadInt32();
}
```

### Asynchronous Call
```cpp
void DataManagerProxy::NotifyAsync(const std::string& event) {
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);  // Asynchronous
    
    data.WriteInterfaceToken(GetDescriptor());
    data.WriteString(event);
    
    // Fire and forget
    Remote()->SendRequest(NOTIFY_EVENT, data, reply, option);
}
```

---

## Service Registration

### Register to SamgrMini
```cpp
// Server side
sptr<MyServiceStub> service = new MyServiceStub();
auto samgr = SamgrMini::GetInstance();

// Register with name
int32_t result = samgr->AddService(u"my.service.name", service->AsObject());
if (result != ERR_NONE) {
    // Handle error
}

// Start IPC loop
IPCSkeleton::JoinWorkThread();
```

### Discover Service
```cpp
// Client side
sptr<IRemoteObject> samgrObj = IPCSkeleton::GetContextObject();
sptr<IServiceRegistry> samgr = iface_cast<IServiceRegistry>(samgrObj);

// Find service by name
sptr<IRemoteObject> obj = samgr->CheckService(u"my.service.name");
if (obj == nullptr) {
    // Service not found
    return;
}

// Cast to interface
sptr<IMyService> service = iface_cast<IMyService>(obj);
```

---

## Client Usage

### Basic Client
```cpp
class ServiceClient {
public:
    bool Connect() {
        sptr<IRemoteObject> samgrObj = IPCSkeleton::GetContextObject();
        if (samgrObj == nullptr) {
            return false;
        }
        
        sptr<IServiceRegistry> samgr = iface_cast<IServiceRegistry>(samgrObj);
        sptr<IRemoteObject> obj = samgr->CheckService(u"my.service");
        
        if (obj == nullptr) {
            return false;
        }
        
        service_ = iface_cast<IMyService>(obj);
        return service_ != nullptr;
    }
    
    int32_t Add(int32_t a, int32_t b) {
        if (service_ == nullptr) {
            return -1;
        }
        return service_->Add(a, b);
    }
    
private:
    sptr<IMyService> service_;
};
```

### Service Retry Logic
```cpp
sptr<IRemoteObject> GetServiceWithRetry(const std::u16string& name, 
                                         int maxRetry = 10,
                                         int delayMs = 500) {
    sptr<IRemoteObject> samgrObj = IPCSkeleton::GetContextObject();
    sptr<IServiceRegistry> samgr = iface_cast<IServiceRegistry>(samgrObj);
    
    for (int i = 0; i < maxRetry; i++) {
        sptr<IRemoteObject> obj = samgr->CheckService(name);
        if (obj != nullptr) {
            return obj;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
    return nullptr;
}
```

---

## Callback Pattern

### Callback Interface
```cpp
class ICallback : public IRemoteBroker {
public:
    enum {
        ON_EVENT = 1,
        ON_DATA_RECEIVED = 2,
    };
    
    virtual int32_t OnEvent(int32_t eventId, const std::string& data) = 0;
    virtual int32_t OnDataReceived(const std::vector<uint8_t>& data) = 0;
    
    DECLARE_INTERFACE_DESCRIPTOR(u"my.ICallback");
};
```

### Callback Stub (Client-side)
```cpp
class CallbackStub : public IRemoteStub<ICallback> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override {
        if (!CheckInterfaceToken(data)) {
            return ERR_INVALID_DATA;
        }
        
        switch (code) {
            case ON_EVENT: {
                int32_t eventId = data.ReadInt32();
                std::string eventData = data.ReadString();
                int32_t result = OnEvent(eventId, eventData);
                reply.WriteInt32(result);
                return ERR_NONE;
            }
            case ON_DATA_RECEIVED: {
                std::vector<uint8_t> buffer;
                data.ReadUInt8Vector(&buffer);
                int32_t result = OnDataReceived(buffer);
                reply.WriteInt32(result);
                return ERR_NONE;
            }
            default:
                return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
    
    int32_t OnEvent(int32_t eventId, const std::string& data) override {
        std::cout << "Event " << eventId << ": " << data << std::endl;
        return ERR_NONE;
    }
    
    int32_t OnDataReceived(const std::vector<uint8_t>& data) override {
        std::cout << "Received " << data.size() << " bytes" << std::endl;
        return ERR_NONE;
    }
};
```

### Server with Callback
```cpp
class DataServiceStub : public IRemoteStub<IDataService> {
public:
    int32_t RegisterCallback(const sptr<IRemoteObject>& callback) override {
        callback_ = callback;
        return ERR_NONE;
    }
    
    void NotifyClients(int32_t eventId) {
        if (callback_ == nullptr) {
            return;
        }
        
        sptr<ICallback> callback = iface_cast<ICallback>(callback_);
        if (callback != nullptr) {
            callback->OnEvent(eventId, "Server notification");
        }
    }
    
private:
    sptr<IRemoteObject> callback_;
};
```

### Client Registration
```cpp
int main() {
    // Create callback implementation
    sptr<CallbackStub> callback = new CallbackStub();
    
    // Connect to service
    sptr<IDataService> service = ConnectToService();
    
    // Register callback
    service->RegisterCallback(callback->AsObject());
    
    // Service can now call back to client
    // Keep running to receive callbacks
    IPCSkeleton::JoinWorkThread();
    
    return 0;
}
```

---

## Common Patterns

### Death Recipient
```cpp
class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    void OnRemoteDied(const wptr<IRemoteObject>& object) override {
        std::cout << "Service died!" << std::endl;
        // Handle service death, reconnect, etc.
    }
};

// Usage
sptr<IRemoteObject> service = ...;
sptr<ServiceDeathRecipient> recipient = new ServiceDeathRecipient();
service->AddDeathRecipient(recipient);
```

### Singleton Service
```cpp
class SingletonService : public IRemoteStub<IService> {
public:
    static sptr<SingletonService> GetInstance() {
        static sptr<SingletonService> instance = new SingletonService();
        return instance;
    }
    
private:
    SingletonService() = default;
};
```

### Interface Cast Helper
```cpp
template<typename INTERFACE>
sptr<INTERFACE> InterfaceCast(const sptr<IRemoteObject>& object) {
    if (object == nullptr) {
        return nullptr;
    }
    return iface_cast<INTERFACE>(object);
}

// Usage
auto service = InterfaceCast<IMyService>(obj);
```

---

## Error Handling

### Common Error Codes
```cpp
ERR_NONE = 0;           // Success
ERR_INVALID_DATA = -1;  // Invalid data
ERR_NULL_OBJECT = -2;   // Null object
ERR_DEAD_OBJECT = -3;   // Remote object died
ERR_TRANSACTION_FAILED = -4;  // Transaction failed
```

### Error Handling Pattern
```cpp
int32_t result = service->SomeMethod();
switch (result) {
    case ERR_NONE:
        // Success
        break;
    case ERR_DEAD_OBJECT:
        // Handle service death
        Reconnect();
        break;
    case ERR_TRANSACTION_FAILED:
        // Retry
        break;
    default:
        // Other error
        break;
}
```

---

## Examples

See complete working examples in:
- `src/rpc_demo/` - Three-process RPC
- `src/rpc_callback/` - Bidirectional RPC with callbacks
