cmake_minimum_required(VERSION 3.10)
project(ashmem LANGUAGES CXX)
add_library(ashmem STATIC
    "ashmem.cpp"
)

target_include_directories(ashmem PUBLIC .)
# musl_ohos_mock supplies <linux/ashmem.h>, which modern Linux headers no longer ship.
auto_target_link_libraries(ashmem PUBLIC misc refbase parcel musl_ohos_mock)
