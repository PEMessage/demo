
cmake_minimum_required(VERSION 3.14)
project(print_cert)

# Use FetchContent to get OpenSSL
include(FetchContent)


# Credit: https://github.com/janbar/openssl-cmake/issues/23#issuecomment-924661972
include(FetchContent)
FetchContent_Declare(
  ssl_content
  URL      https://github.com/janbar/openssl-cmake/archive/refs/tags/1.1.1w-20231130.zip
  URL_HASH MD5=0f997c8e1c3c69a56964eca896e66af5
)
FetchContent_MakeAvailable(ssl_content)
add_library(OpenSSL::SSL ALIAS ssl)
add_library(OpenSSL::Crypto ALIAS crypto)

# Add executable
add_executable(print_cert main.c)

# Link OpenSSL libraries (this will not work as-is - see explanation)
target_link_libraries(print_cert PRIVATE
  OpenSSL::SSL
  OpenSSL::Crypto
)
