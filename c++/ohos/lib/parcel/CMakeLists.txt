cmake_minimum_required(VERSION 3.10)
project(parcel LANGUAGES CXX)
add_library(parcel STATIC
    "parcel.cpp"
)

target_include_directories(parcel PUBLIC .)
auto_target_link_libraries(parcel PUBLIC misc refbase libsec nocopyable)
