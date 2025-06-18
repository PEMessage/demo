
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
