# OpenHarmony CMake 学习项目 - 代码地图

## 项目概述

这是一个用于学习和调试OpenHarmony基础库的CMake构建项目。它将OpenHarmony的核心C++库抽取出来，使其可以在标准Linux环境下使用CMake编译和调试，方便开发者理解OHOS底层架构。

## 项目结构

```
/home/zhuojw/demo_new/c++/ohos/
├── lib/                    # OpenHarmony基础库 (22个库)
│   ├── hilog/             # 日志框架
│   ├── ipc/               # 进程间通信
│   ├── parcel/            # 数据序列化
│   ├── refbase/           # 引用计数基类
│   ├── ashmem/            # 匿名共享内存
│   ├── eventhandler/      # 事件处理框架
│   ├── ffrt/              # 异步任务调度
│   ├── hitrace/           # 分布式追踪
│   ├── timer/             # 定时器管理
│   ├── syspara/           # 系统参数访问
│   ├── thread_ex/         # 线程扩展
│   ├── cjson/             # JSON解析
│   └── ...                # 其他工具库
├── src/                    # 示例程序 (13个Demo)
│   ├── hilog_demo/        # 日志使用示例
│   ├── parcel_demo/       # Parcel序列化示例
│   ├── message_parcel_demo/  # IPC消息传递示例
│   ├── rpc_server_demo/   # RPC服务示例
│   ├── timer_demo/        # 定时器示例
│   └── ...
├── include.cmake          # CMake公共配置
├── run                    # 构建运行脚本
└── rule.yaml              # 代码转换规则
```

## 快速开始

### 构建项目

```bash
# 构建并运行 hilog_demo
./run src/hilog_demo

# 仅构建
./run src/hilog_demo build

# 清理
./run src/hilog_demo clean

# 传递参数给程序
./run src/hilog_demo run -- arg1 arg2
```

### 添加新库

1. 在 `lib/` 下创建库目录
2. 添加 `CMakeLists.txt`
3. 使用 `auto_target_link_libraries()` 声明依赖

### 添加新Demo

1. 在 `src/` 下创建目录
2. 添加 `main.cpp` 和 `CMakeLists.txt`
3. 使用 `./run src/your_demo` 构建运行

## 库目录详细地图

### 核心框架库

| 库 | 职责 | 详细地图 |
|---|------|---------|
| `lib/hilog/` | 高性能日志框架，支持多级日志、隐私保护、流控 | [查看](lib/hilog/codemap.md) |
| `lib/ipc/` | 进程间通信框架，支持Proxy-Stub模式、RPC跨设备通信 | [查看](lib/ipc/codemap.md) |
| `lib/parcel/` | 二进制数据序列化，支持FD传递、对象引用 | [查看](lib/parcel/codemap.md) |
| `lib/refbase/` | 引用计数基类，提供sptr/wptr智能指针 | [查看](lib/refbase/codemap.md) |

### 系统服务库

| 库 | 职责 | 详细地图 |
|---|------|---------|
| `lib/ashmem/` | 匿名共享内存，用于零拷贝IPC数据传输 | [查看](lib/ashmem/codemap.md) |
| `lib/eventhandler/` | 事件循环框架，类似Android Looper/Handler | [查看](lib/eventhandler/codemap.md) |
| `lib/ffrt/` | 异步任务调度框架(Fiber Flow Runtime) | [查看](lib/ffrt/codemap.md) |
| `lib/hitrace/` | 分布式追踪框架，支持跨进程/跨设备追踪 | [查看](lib/hitrace/codemap.md) |
| `lib/timer/` | 定时器管理，基于timerfd+epoll | [查看](lib/timer/codemap.md) |
| `lib/syspara/` | 系统参数访问接口 | [查看](lib/syspara/codemap.md) |
| `lib/thread_ex/` | POSIX线程封装，支持优先级和命名 | [查看](lib/thread_ex/codemap.md) |

### 工具库

| 库 | 职责 | 详细地图 |
|---|------|---------|
| `lib/cjson/` | JSON解析/生成库 | [查看](lib/cjson/codemap.md) |
| `lib/misc/` | 核心工具：错误码、单例、线程安全容器 | [查看](lib/misc/codemap.md) |
| `lib/nocopyable/` | 禁用拷贝的基类宏 | [查看](lib/nocopyable/codemap.md) |
| `lib/string_ex/` | 字符串操作扩展 | [查看](lib/string_ex/codemap.md) |
| `lib/file_ex/` | 文件操作扩展 | [查看](lib/file_ex/codemap.md) |
| `lib/directory_ex/` | 目录操作扩展 | [查看](lib/directory_ex/codemap.md) |
| `lib/unicode_ex/` | UTF-8/UTF-16编码转换 | [查看](lib/unicode_ex/codemap.md) |
| `lib/init_utils/` | Init进程工具函数 | [查看](lib/init_utils/codemap.md) |
| `lib/libsec/` | 安全函数库(自动获取) | [查看](lib/libsec/codemap.md) |
| `lib/musl_ohos_mock/` | Musl libc兼容层 | [查看](lib/musl_ohos_mock/codemap.md) |

## Demo目录详细地图

### 日志与追踪

| Demo | 职责 | 详细地图 |
|------|------|---------|
| `src/hilog_demo/` | HiLog基础使用：日志级别、标签定义、格式化 | [查看](src/hilog_demo/codemap.md) |

### IPC/RPC 学习路径

| Demo | 职责 | 详细地图 |
|------|------|---------|
| `src/parcel_demo/` | 基础Parcel序列化：读写基本类型、字符串、数组 | [查看](src/parcel_demo/codemap.md) |
| `src/message_parcel_demo/` | IPC消息传递：MessageParcel、FD传递 | [查看](src/message_parcel_demo/codemap.md) |
| `src/rpc_server_demo/` | 完整RPC示例：服务注册、代理/存根 | [查看](src/rpc_server_demo/codemap.md) |
| `src/process_skeleton_demo/` | 进程骨架管理：对象生命周期 | [查看](src/process_skeleton_demo/codemap.md) |
| `src/persistable_bundle_demo/` | 持久化Bundle数据 | [查看](src/persistable_bundle_demo/codemap.md) |

### 内存管理

| Demo | 职责 | 详细地图 |
|------|------|---------|
| `src/ashmem_demo/` | 匿名共享内存使用 | [查看](src/ashmem_demo/codemap.md) |
| `src/refbase_demo/` | 引用计数与智能指针 | [查看](src/refbase_demo/codemap.md) |

### 定时器与线程

| Demo | 职责 | 详细地图 |
|------|------|---------|
| `src/timer_demo/` | 基础定时器使用 | [查看](src/timer_demo/codemap.md) |
| `src/timer_node_demo/` | 定时器节点管理 | [查看](src/timer_node_demo/codemap.md) |
| `src/timer_node_demo_v2/` | 定时器节点高级用法 | [查看](src/timer_node_demo_v2/codemap.md) |
| `src/thread_ex_demo/` | 线程扩展使用 | [查看](src/thread_ex_demo/codemap.md) |
| `src/thread_ex_timer/` | 线程与定时器结合 | [查看](src/thread_ex_timer/codemap.md) |

### 所有Demo总览

详见 [src/codemap.md](src/codemap.md)

## 核心设计模式

### 1. 智能指针系统 (RefBase)

```cpp
// 所有OHOS对象继承RefBase
class MyClass : public RefBase { ... };

// 使用sptr管理强引用
sptr<MyClass> obj = new MyClass();

// 使用wptr管理弱引用
wptr<MyClass> weak = obj;
sptr<MyClass> strong = weak.promote();  // 可能返回nullptr
```

### 2. IPC Proxy-Stub 模式

```cpp
// 定义接口
class IMyService : public IRemoteBroker {
public:
    virtual int DoSomething(int arg) = 0;
};

// 服务端实现Stub
class MyServiceStub : public IRemoteStub<IMyService> {
    int DoSomething(int arg) override { ... }
};

// 客户端使用Proxy
sptr<IMyService> proxy = ...;  // 从IPC获取
proxy->DoSomething(42);
```

### 3. 自动库链接

通过 `include.cmake` 提供的宏自动查找和链接库：

```cmake
# 在Demo的CMakeLists.txt中
include("../../include.cmake")
add_executable(app main.cpp)

# 自动查找lib/hilog目录并链接
auto_target_link_libraries(app PRIVATE hilog)

# 可链接多个库
auto_target_link_libraries(app PRIVATE hilog ipc refbase)
```

### 4. 日志标签定义

```cpp
// 定义日志标签
static constexpr OHOS::HiviewDFX::HiLogLabel LOG_LABEL = {
    LOG_APP,           // 日志类型
    0xD000000 + 1,    // Domain
    "MyModule"        // Tag
};

// 使用宏输出日志
HiLog::Info(LOG_LABEL, "Message: %{public}s", str);
```

## 依赖关系图

```
                    ┌─────────────┐
                    │   src/*     │  (Demo应用)
                    └──────┬──────┘
                           │ 使用
                           ▼
        ┌──────────────────────────────────────┐
        │              lib/                     │
        │  ┌───────┐  ┌───────┐  ┌─────────┐  │
        │  │ hilog │  │  ipc  │  │ parcel  │  │
        │  └───┬───┘  └───┬───┘  └────┬────┘  │
        │      │          │           │       │
        │  ┌───┴───┐  ┌───┴───┐  ┌────┴────┐  │
        │  │ libsec│  │refbase│  │  misc   │  │
        │  └───────┘  └───┬───┘  └─────────┘  │
        │                 │                   │
        │            ┌────┴────┐              │
        │            │  misc   │              │
        │            └─────────┘              │
        └──────────────────────────────────────┘
```

## 调试技巧

### 1. 使用 compile_commands.json

构建后生成，用于clangd/LSP代码补全：

```bash
# compile_commands.json 会自动链接到项目根目录
ls -la compile_commands.json
```

### 2. 调试单个库

```bash
# 构建特定库
./run src/hilog_demo build

# 使用gdb调试
gdb ./out/hilog_demo/app
```

### 3. 启用调试模式

refbase支持调试模式追踪引用计数：

```cmake
# 链接调试版本的refbase
auto_target_link_libraries(app PRIVATE refbase+debug)
```

## 关键配置文件

| 文件 | 职责 |
|-----|------|
| `include.cmake` | CMake公共配置，提供`auto_target_link_libraries`宏和调试选项 |
| `run` | 构建运行脚本，简化CMake命令 |
| `rule.yaml` | 代码转换规则（ast-grep格式） |
| `.project_root` | 项目根标记，用于`find_project_root`函数 |

## 学习路径建议

1. **入门**：从 `src/hilog_demo` 开始，了解日志系统
2. **内存管理**：学习 `src/refbase_demo`，理解sptr/wptr
3. **序列化**：学习 `src/parcel_demo`，理解数据打包
4. **IPC基础**：学习 `src/message_parcel_demo`，理解跨进程通信
5. **完整RPC**：学习 `src/rpc_server_demo`，理解服务架构
6. **高级主题**：探索 `lib/eventhandler`、`lib/ffrt`、`lib/hitrace`

## 相关资源

- [OpenHarmony官方文档](https://gitee.com/openharmony/docs)
- [OpenHarmony源码](https://gitee.com/openharmony)
- [lib/ 库目录地图](lib/)
- [src/ 示例目录地图](src/)
