# OpenHarmony IPC/RPC Learning Project

## Project Overview

This is a CMake-based learning project for OpenHarmony (OHOS) core libraries. It extracts fundamental C++ libraries from OpenHarmony to enable compilation and debugging on standard Linux environments, making it easier to understand the OHOS architecture.

## Architecture

```
ohos/
├── lib/                    # 22 OpenHarmony base libraries
│   ├── hilog/             # High-performance logging
│   ├── ipc/               # Inter-process communication (Binder-based)
│   ├── parcel/            # Binary data serialization
│   ├── refbase/           # Reference counting (sptr/wptr)
│   ├── ashmem/            # Anonymous shared memory
│   ├── eventhandler/      # Event loop framework
│   ├── ffrt/              # Async task scheduling
│   ├── hitrace/           # Distributed tracing
│   ├── timer/             # Timer management
│   ├── syspara/           # System parameters
│   └── ...
├── src/                    # Demo applications
│   ├── rpc_demo/          # Three-process RPC demonstration
│   ├── rpc_callback/      # Bidirectional RPC with callbacks
│   ├── hilog_demo/        # Logging examples
│   └── ...
├── include.cmake          # CMake common configuration
└── run                    # Build/run script
```

## Quick Start

```bash
# Build and run a demo (specify the directory containing CMakeLists.txt)
./run src/hilog_demo

# Build only
./run src/hilog_demo build

# Clean
./run src/hilog_demo clean
```

## RPC Examples

### Three-Process Architecture

The `rpc_demo` demonstrates the standard OHOS service architecture:

1. **SamgrServer** - Service manager (like Android's ServiceManager)
2. **DemoServiceServer** - Business service implementation
3. **DemoClient** - Client that calls the service

```bash
# Terminal 1: Start Service Manager
./run src/samgr_server run

# Terminal 2: Start Business Service  
./run src/rpc_demo/server run

# Terminal 3: Run Client
./run src/rpc_demo/client run
```

### Bidirectional RPC with Callbacks

The `rpc_callback` demonstrates callback mechanism:

1. Client implements a Callback service
2. Client registers Callback to Server
3. Server can call Client's methods via the Callback proxy

```bash
# Same three-process setup
./run src/rpc_callback/server run
./run src/rpc_callback/client run
```

## Key Libraries

| Library | Purpose | Key Classes |
|---------|---------|-------------|
| `lib/ipc` | IPC/RPC framework | `IRemoteBroker`, `IRemoteProxy`, `IRemoteStub`, `IPCObjectProxy`, `IPCObjectStub`, `IPCSkeleton` |
| `lib/parcel` | Data serialization | `MessageParcel`, `Parcelable` |
| `lib/refbase` | Smart pointers | `RefBase`, `sptr`, `wptr` |
| `lib/hilog` | Logging | `HiLog`, `HiLogLabel` |

## Design Patterns

### Proxy-Stub Pattern
```
Client -> IRemoteProxy -> Binder Driver -> IRemoteStub -> Service
```

### Service Registration Flow
```
Service -> SamgrServer.AddService(name, stub)
Client -> SamgrServer.CheckService(name) -> handle
Client -> Direct IPC call via handle
```

## Adding New Demos

1. Create directory: `src/your_demo/`
2. Add `CMakeLists.txt` in that directory:
```cmake
cmake_minimum_required(VERSION 3.10)
project(your_demo)
include("../../include.cmake")

add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE ipc refbase hilog)
```
3. Build: `./run src/your_demo build`
4. Run: `./run src/your_demo run`

## Requirements

- Linux with Binder driver (`/dev/binder`)
- CMake 3.10+
- C++17 compiler

## References

- [Code Map](codemap_zh.md) - Detailed code structure
- [RPC Skill](skills/rpc/SKILL.md) - RPC implementation patterns
