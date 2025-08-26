cmake_minimum_required(VERSION 3.10)
project(eventhandler LANGUAGES CXX C)
add_library(eventhandler STATIC
    frameworks/eventhandler/src/file_descriptor_listener.cpp
    frameworks/eventhandler/src/none_io_waiter.cpp
    frameworks/eventhandler/src/inner_event.cpp
    frameworks/eventhandler/src/event_handler.cpp
    frameworks/eventhandler/src/event_queue.cpp
    frameworks/eventhandler/src/event_queue_ffrt.cpp
    frameworks/eventhandler/src/deamon_io_waiter.cpp
    frameworks/eventhandler/src/ffrt_descriptor_listener.cpp
    frameworks/eventhandler/src/event_runner.cpp
    frameworks/eventhandler/src/event_queue_base.cpp
    frameworks/eventhandler/src/native_implement_eventhandler.cpp
    frameworks/eventhandler/src/epoll_io_waiter.cpp
)

target_include_directories(eventhandler PUBLIC
    interfaces/inner_api
    frameworks/eventhandler/include
)

# For getproctid
target_compile_definitions(eventhandler PRIVATE _DEFAULT_SOURCE)
auto_target_link_libraries(eventhandler PUBLIC libsec nocopyable misc hilog hitrace syspara ffrt)
