
set(LINKER_OPTION "")
# Append each linker option separately
list(APPEND LINKER_OPTION "-Wl,--cmse-implib")
list(APPEND LINKER_OPTION "-Wl,--sort-section=alignment")
list(APPEND LINKER_OPTION "-Wl,--out-implib=cmse_import.o")

# Join the list into a single string for use in linker flags
string(JOIN " " LINKER_OPTION ${LINKER_OPTION})

message(STATUS "Linker options: ${LINKER_OPTION}")
