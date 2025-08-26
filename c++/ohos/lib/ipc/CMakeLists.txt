cmake_minimum_required(VERSION 3.10)
project(ipc LANGUAGES CXX C)
add_library(ipc STATIC
    ipc/native/src/core/framework/source/iremote_broker.cpp
    ipc/native/src/core/framework/source/process_skeleton.cpp
    ipc/native/src/core/framework/source/ipc_payload_statistics.cpp
    ipc/native/src/core/framework/source/ipc_process_skeleton.cpp
    ipc/native/src/core/framework/source/binder_debug.cpp
    ipc/native/src/core/framework/source/message_parcel.cpp
    ipc/native/src/core/framework/source/ipc_skeleton.cpp
    ipc/native/src/core/framework/source/ipc_file_descriptor.cpp
    ipc/native/src/core/framework/source/iremote_object.cpp
    ipc/native/src/core/framework/source/comm_auth_info.cpp
    ipc/native/src/core/framework/source/ipc_object_stub.cpp
    ipc/native/src/core/framework/source/message_option.cpp
    ipc/native/src/core/framework/source/buffer_object.cpp
    ipc/native/src/core/framework/source/ipc_trace.cpp
    ipc/native/src/core/framework/source/ipc_thread_skeleton.cpp
    ipc/native/src/core/framework/source/stub_refcount_object.cpp
    ipc/native/src/core/framework/source/ipc_object_proxy.cpp
    ipc/native/src/core/framework/source/ipc_payload_statistics_impl.cpp
    ipc/native/src/core/framework/source/ipc_workthread.cpp
    ipc/native/src/core/framework/source/peer_holder.cpp
    ipc/native/src/core/framework/source/ipc_thread_pool.cpp

    utils/src/rpc_session_handle.c

    ipc/native/src/core/invoker/source/invoker_rawdata.cpp
    ipc/native/src/core/invoker/source/invoker_factory.cpp
    ipc/native/src/core/invoker/source/binder_invoker.cpp
    ipc/native/src/core/invoker/source/hitrace_invoker.cpp
    ipc/native/src/core/invoker/source/binder_connector.cpp

    ipc/native/c/adapter/access_token/src/access_token_adapter.c


    ipc/native/c/manager/src/rpc_log.c
)

target_include_directories(ipc PUBLIC
    interfaces/innerkits/ipc_core/include
    interfaces/innerkits/c/ipc/include

    ipc/native/src/core/framework/include
    ipc/native/src/core/invoker/include

    utils/include

    ipc/native/c/adapter/access_token/include
    ipc/native/c/manager/include
)

target_compile_definitions(ipc PRIVATE CONFIG_IPC_SINGLE)
auto_target_link_libraries(ipc PUBLIC libsec misc ashmem hilog string_ex ffrt hitrace musl_ohos_mock)
