
message("")
set(DO_NOTHING "-----------Test 1-------------------------")
set(MY_VAR "Hello")

# Correct way - no $ in if() statements
if(MY_VAR STREQUAL "Hello")
    message(STATUS "If branch") # <--- this 
else()
    message(STATUS "Else branch")
endif()

if(${MY_VAR} STREQUAL "Hello")
    message(STATUS "If branch") # <--- this
else()
    message(STATUS "Else branch")
endif()

set(DO_NOTHING "-----------Test 2-------------------------")

if(MY_VAR)
    message(STATUS "If branch") # <--- this
else()
    message(STATUS "Else branch") 
endif()

if(${MY_VAR})
    message(STATUS "If branch")
else()
    message(STATUS "Else branch") # <--- this
endif()

if("${MY_VAR}")
    message(STATUS "If branch")
else()
    message(STATUS "Else branch")  # <--- this
    # ``if(<string>)``                                                   
    #  A quoted string always evaluates to false unless:                 
    #     * The string's value is one of the true constants, or             
    #     * Policy ``CMP0054`` is not set to ``NEW`` and the string's value 
    #       happens to be a variable name that is affected by ``CMP0054``'s 
    #       behavior.                                                       
endif()


set(DO_NOTHING "-----------Set Policy CMP0054-------------")
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

set(Hello "Hello")
set(DO_NOTHING "-----------After Policy CMP0054-----------")


if(${MY_VAR})
    message(STATUS "If branch") # <--- this
else()
    message(STATUS "Else branch")
endif()

if("${MY_VAR}")
    message(STATUS "If branch")
else()
    message(STATUS "Else branch") # <--- this 
    # though predicate commands like if receive quoting information that isn’t shown
endif()
