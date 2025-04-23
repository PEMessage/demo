
message("")
message("set(MY_VAR \"Hello\")")
set(MY_VAR "Hello")
message("")

# Correct way - no $ in if() statements
message("if(MY_VAR STREQUAL \"Hello\")")
if(MY_VAR STREQUAL "Hello")
    message(STATUS "If branch") # <--- this 
else()
    message(STATUS "Else branch")
endif()
message("")


message("if(\${MY_VAR} STREQUAL \"Hello\")")
if(${MY_VAR} STREQUAL "Hello")
    message(STATUS "If branch") # <--- this
else()
    message(STATUS "Else branch")
endif()
message("")


message("if(\${MY_VAR})")
if(${MY_VAR})
    message(STATUS "If branch")
else()
    message(STATUS "Else branch") # <--- this
endif()
message("")

set(Hello "Hello")
message("set(Hello \"Hello\") # now \${MY_VAR} -> Hello -> \"Hello\"")
message("")

message("if(\${MY_VAR}) # repeat, but different result")
if(${MY_VAR})
    message(STATUS "If branch") # <--- this
else()
    message(STATUS "Else branch") 
endif()
