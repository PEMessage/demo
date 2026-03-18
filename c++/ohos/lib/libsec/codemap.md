# lib/libsec/

## Responsibility

Provides bounds-checking safe C string and memory functions (secureC library). Fetched at build time from OpenHarmony's third-party bounds-checking-function repository.

## Design

**Key Pattern:** External dependency fetched via CMake FetchContent

**Source:** https://gitee.com/openharmony/third_party_bounds_checking_function.git

**Components:**
- Safe string functions (strncpy_s, strcpy_s, etc.)
- Safe memory functions (memcpy_s, memmove_s, etc.)
- Header files in `code/include/`
- Implementation in `code/src/`

## Flow

1. CMake fetches the secureC library at configure time
2. Sources are placed in `code/` directory (gitignored)
3. Static library `libsec` is built from fetched sources
4. Other libraries link against `libsec` for safe operations

## Integration

**Dependencies:** 
- External: third_party_bounds_checking_function (fetched at build time)

**Used By:**
- lib/directory_ex
- lib/init_utils
- Any code needing safe string/memory operations

**CMake:** Static library with FetchContent

**Git:** `code/` directory is gitignored (fetched content)
