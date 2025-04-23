message("Hi")
message("before find_package(helloworld)")
find_package(helloworld
    HINTS cmake # must give it a path to find helloworldConfig.cmake
    )
message("After find_package(helloworld)")

message("Hello World")
