# src/ - OpenHarmony C++ Demo Projects

## Responsibility

This directory contains standalone demo applications that demonstrate how to use the OpenHarmony system libraries located in `/home/zhuojw/demo_new/c++/ohos/lib/`. Each demo showcases specific OpenHarmony APIs and usage patterns.

### Demo Categories

| Demo | Library | Description |
|------|---------|-------------|
| **hilog_demo** | hilog | HiLog logging framework usage |
| **parcel_demo** | parcel | Basic Parcel serialization/deserialization |
| **message_parcel_demo** | ipc + ashmem | IPC MessageParcel for inter-process communication |
| **rpc_server_demo** | ipc + ashmem | Full RPC server with stub/proxy pattern |
| **process_skeleton_demo** | ipc + ashmem + misc | ProcessSkeleton IPC management APIs |
| **ashmem_demo** | ashmem | Anonymous Shared Memory operations |
| **refbase_demo** | refbase + hilog | Smart pointer (sptr/wptr) reference counting |
| **persistable_bundle_demo** | parcel + misc + string_ex | Key-value bundle persistence with Parcel |
| **timer_demo** | timer | Timer-based callback scheduling |
| **timer_node_demo** | timer + nocopyable | Timer-driven node device simulation |
| **timer_node_demo_v2** | timer + nocopyable | Enhanced timer node with manager pattern |
| **thread_ex_demo** | thread_ex | Thread abstraction and lifecycle management |
| **thread_ex_timer** | thread_ex + misc | Thread-based timer daemon implementation |

## Design

### Common Patterns

1. **Library Inclusion via `auto_target_link_libraries`**
   - All demos use the custom CMake macro `auto_target_link_libraries()` from `include.cmake`
   - Automatically finds and links libraries from `/home/zhuojw/demo_new/c++/ohos/lib/`
   - Example: `auto_target_link_libraries(app PRIVATE hilog)`

2. **Namespace Convention**
   - All OpenHarmony APIs use the `OHOS` namespace
   - Utility classes use `OHOS::Utils` namespace
   - Some demos define their own namespaces (e.g., `Node::`)

3. **Reference Counting with `sptr`/`wptr`**
   - Objects inheriting from `RefBase` use smart pointers
   - `sptr<T>` for strong references, `wptr<T>` for weak references
   - Automatic memory management demonstrated in `refbase_demo`

4. **Parcel Pattern for Serialization**
   - Data marshalling via `Marshalling()` and `Unmarshalling()`
   - Used for IPC and persistent storage
   - Supports primitives, strings, vectors, and custom `Parcelable` objects

5. **Nocopyable Pattern**
   - Classes use `DISALLOW_COPY_AND_MOVE(ClassName)` macro
   - Prevents accidental copying of resource-managing objects

## Flow

### Build System Flow

```
CMakeLists.txt
├── include("${ROOT_DIR}/include.cmake")  # Project-wide configuration
├── add_executable(app main.cpp)    # Define target
└── auto_target_link_libraries()    # Link OHOS libraries
    ├── find library in lib/
    ├── add_subdirectory()
    └── target_link_libraries()
```

### Demo Execution Flow

1. **Configure & Build**
   ```bash
   cd /home/zhuojw/demo_new/c++/ohos/src/<demo_name>
   mkdir build && cd build
   cmake ..
   make
   ```

2. **Run** (most demos produce a single `app` executable)
   ```bash
   ./app
   ```

3. **Special Requirements**
   - `ashmem_demo`: Requires `chmod 0666 /dev/ashmem` before running
   - `rpc_server_demo`: Demonstrates IPC stub/proxy architecture
   - Timer demos: Run for extended periods to demonstrate callbacks

## Integration

### Library Dependencies

```
src/<demo>/
├── CMakeLists.txt          # Uses auto_target_link_libraries
├── main.cpp                # Demo implementation
└── [supporting files]      # Headers, implementation files

Links to:
lib/
├── hilog/                  # Logging framework
├── parcel/                 # Serialization
├── ipc/                    # Inter-process communication
├── ashmem/                 # Shared memory
├── refbase/                # Reference counting
├── timer/                  # Timer utilities
├── thread_ex/              # Thread abstraction
├── nocopyable/             # Copy prevention macro
└── [other utilities]
```

### Key Library APIs Demonstrated

| Library | Key Classes/Functions |
|---------|----------------------|
| hilog | `HiLogLabel`, `HILOG_IMPL`, `LOG_APP`, `LOG_FATAL` |
| parcel | `Parcel`, `WriteInt32()`, `ReadString()`, `Marshalling()` |
| ipc | `MessageParcel`, `MessageOption`, `IRemoteStub`, `IRemoteProxy` |
| ashmem | `Ashmem::CreateAshmem()`, `MapReadAndWriteAshmem()` |
| refbase | `RefBase`, `sptr<>`, `wptr<>`, `MakeSptr()` |
| timer | `Utils::Timer`, `Register()`, `Setup()`, `Shutdown()` |
| thread_ex | `Thread`, `Start()`, `Run()`, `IsRunning()` |

## Running the Demos

### Quick Start

```bash
cd /home/zhuojw/demo_new/c++/ohos/src/hilog_demo
mkdir -p build && cd build
cmake ..
make
./app
```

### Demo-Specific Notes

- **hilog_demo**: Shows fatal-level logging with function/line macros
- **parcel_demo**: Visual hex dump of parcel data; demonstrates alignment/padding
- **message_parcel_demo**: Shows IPC-ready MessageParcel with interface tokens
- **rpc_server_demo**: Complete RPC with `IRpcFooTest` interface, stub, and proxy
- **process_skeleton_demo**: Single-process IPC object management demo
- **ashmem_demo**: Creates 4KB shared memory region, writes/reads data
- **refbase_demo**: Demonstrates sptr reference counting and lifecycle
- **persistable_bundle_demo**: Key-value store with type-safe serialization
- **timer_demo**: Two periodic callbacks (1s and 2s intervals)
- **timer_node_demo**: Simulates LED-like device with blink/const/duty modes
- **timer_node_demo_v2**: Multi-device management with slot-based configuration
- **thread_ex_demo**: Self-terminating thread after 5 iterations
- **thread_ex_timer**: Thread-safe timer daemon with message queue
