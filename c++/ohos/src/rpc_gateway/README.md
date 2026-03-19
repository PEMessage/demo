# RPC Gateway Demo

这是一个 3+1 架构的 RPC 网关演示，展示了如何通过 Gateway 中转回调，实现 Server 与 Client 之间的间接通信。

## 架构

```
┌─────────────┐         ┌───────────────┐         ┌───────────────┐
│   Client    │◄───────►│    Gateway    │◄───────►│    Server     │
│             │         │               │         │  (Backend)    │
│             │         │  - 注册Client  │         │               │
│ IEventCallback│       │  - 注册到Server│         │ IEventCallback│
│ (接收回调)   │         │  - 事件过滤    │         │ (触发事件)    │
└─────────────┘         └───────────────┘         └───────────────┘
        ▲                                                  │
        │                                                  │
        │           Callback Registration Flow             │
        │                                                  │
        │     Client -> Gateway -> Server (注册)           │
        │                                                  │
        │           Event Flow (Filtered)                  │
        └──────────────────────────────────────────────────┘
                Server -> Gateway -> Filter -> Client
```

## 特性

1. **回调注册链**: Client -> Gateway -> Server
2. **事件转发链**: Server -> Gateway -> Client
3. **事件过滤**: Gateway 的 OnEvent 可以按事件类型过滤
4. **隔离性**: Server 不能直接访问 Client，必须通过 Gateway

## 接口定义

```cpp
// 回调接口
virtual ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) = 0;
```

## 启动顺序

### 1. 启动 SamgrServer (服务注册中心)
```bash
./run src/samgr_server run
```

### 2. 启动 BackendServer
```bash
./run src/rpc_gateway/server run
```

### 3. 启动 Gateway
```bash
./run src/rpc_gateway/gateway run
```

### 4. 启动 Client (可启动多个)
```bash
./run src/rpc_gateway/client run
```

## 事件过滤演示

Client 在注册回调时可以指定过滤的事件类型：

```cpp
// Client 代码示例
std::vector<int32_t> filterTypes = {1000, 1002, 1004};  // 只接收这些事件
gatewayProxy->RegisterClientCallback(callback, filterTypes);
```

Server 每 5 秒触发递增的事件 ID (1000, 1001, 1002, ...)，Client 只会收到指定过滤的事件。

## 文件结构

```
src/rpc_gateway/
├── rpc_common/                    # 共享代码
│   ├── event_callback_interface.h  # IEventCallback 接口
│   ├── event_callback_proxy.h/cpp  # EventCallback Proxy/Stub
│   ├── gateway_service_interface.h # Gateway 服务接口
│   ├── gateway_service_proxy.h/cpp # Gateway Proxy (Client 使用)
│   ├── gateway_service_stub.h/cpp  # Gateway Stub (Gateway 服务)
│   ├── backend_service_interface.h # Backend 服务接口
│   ├── backend_service_proxy.h/cpp # Backend Proxy (Gateway 使用)
│   └── backend_service_stub.h/cpp  # Backend Stub (Server 服务)
├── server/                        # Backend Server
│   ├── main.cpp
│   └── CMakeLists.txt
├── gateway/                       # Gateway Service
│   ├── main.cpp
│   └── CMakeLists.txt
└── client/                        # Client
    ├── main.cpp
    └── CMakeLists.txt
```

## 构建

```bash
# 构建 Server
./run src/rpc_gateway/server build

# 构建 Gateway
./run src/rpc_gateway/gateway build

# 构建 Client
./run src/rpc_gateway/client build
```

## 关键设计

### 1. Gateway 作为中介
- Gateway 向 Server 注册自己的回调
- Client 向 Gateway 注册回调
- Server 触发事件时，实际上是发给 Gateway
- Gateway 根据过滤条件决定是否转发给 Client

### 2. 死亡回调处理
- Gateway 为每个 Client 回调注册死亡回调
- Client 意外退出时，Gateway 自动清理
- Gateway 退出时，自动从 Server 注销

### 3. 事件过滤
- 支持按事件类型过滤
- 支持多 Client，每个 Client 可配置不同的过滤条件
- 空过滤列表表示接收所有事件
