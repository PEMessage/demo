
cmake_minimum_required(VERSION 3.11)
project(print_der)

include(FetchContent)

# FetchContent_Declare(
#   googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG        703bd9caab50b139428cea1aaff9974ebee5742e # release-1.10.0
# )

FetchContent_Declare(
  mbedtls
  GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
  GIT_TAG        2d1f26037f586cf727fb3b334e123e765f21cf0f 
)

FetchContent_MakeAvailable(mbedtls)


add_executable(print_der main.c)
target_link_libraries(print_der
  PRIVATE
    mbedcrypto
    mbedx509
    mbedtls
)

