
set(LINKER_OPTION "")
# Append each linker option separately
list(APPEND LINKER_OPTION "-Wl,--cmse-implib")
list(APPEND LINKER_OPTION "-Wl,--sort-section=alignment")
list(APPEND LINKER_OPTION "-Wl,--out-implib=cmse_import.o dummy")

# Join the list into a single string for use in linker flags
string(JOIN " " LINKER_OPTION ${LINKER_OPTION})

message(STATUS "Linker options: ${LINKER_OPTION}")


set(MPS2AN505_MCU_FLAGS "")
list(APPEND MPS2AN505_MCU_FLAGS -mcpu=cortex-m33 -mthumb -mcmse)
list(APPEND MPS2AN505_MCU_FLAGS -DARMCM33_DSP_FP_TZ -DC_SECURE_CODE)
string(JOIN " " MPS2AN505_MCU_FLAGS ${MPS2AN505_MCU_FLAGS})
message(STATUS "MCUFLAGS options: ${MPS2AN505_MCU_FLAGS}")

message(STATUS "")
message(STATUS "")
message(STATUS "")
message(STATUS "")

function(my_function)
    # Get the number of arguments
    list(LENGTH ARGV arg_count)
    message(STATUS "Function received ${arg_count} arguments:")

    math(EXPR last_index "${arg_count} - 1")
    # Loop through and print each argument
    foreach(arg_index RANGE 0 "${last_index}")
        # message(STATUS "  Argument ${arg_index}")
        list(GET ARGV ${arg_index} current_arg)
        message(STATUS "  Argument ${arg_index}: '${current_arg}'")
    endforeach()
endfunction()


set(TEST_LIST "a" "b" "c" "d")
# Alternative: Pass as a single quoted argument (if function expects a single list)
# Both case will turn LIST into ARGV
my_function("${TEST_LIST}")
my_function(${TEST_LIST})
# same as this
my_function("a" "b" "c" "d")

set(TEST_LIST "a a" "b" "c" "d") # contain space in args seem like work fine
my_function("${TEST_LIST}")
my_function(${TEST_LIST})

