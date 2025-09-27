cmake_minimum_required(VERSION 3.10)
project(timer LANGUAGES CXX)
add_library(timer STATIC
    timer.cpp
    timer_event_handler.cpp

    event_demultiplexer.cpp
    event_handler.cpp
    event_reactor.cpp
)

target_include_directories(timer PUBLIC .)
auto_target_link_libraries(timer PRIVATE misc)
