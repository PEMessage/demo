# lib/file_ex/

## Responsibility

Provides extended file I/O operations including reading/writing strings and buffers, file existence checks, and content searching. Includes both file path and file descriptor APIs.

## Design

**Key Function Groups:**

1. **String File I/O:**
   - `LoadStringFromFile()` - Read entire file to std::string (max 32MB)
   - `SaveStringToFile()` - Write std::string to file
   - `LoadStringFromFd()`, `SaveStringToFd()` - FD-based operations

2. **Binary File I/O:**
   - `LoadBufferFromFile()` - Read file to std::vector<char>
   - `SaveBufferToFile()` - Write buffer to file

3. **File Inspection:**
   - `FileExists()` - Check file accessibility
   - `StringExistsInFile()` - Search for substring (case-sensitive option)
   - `CountStrInFile()` - Count occurrences of substring

4. **Rust Compatibility:**
   - All functions have `RustXxx` variants when `UTILS_CXX_RUST` defined
   - Bridges C++ std::string/vector with Rust types

**Safety Features:**
- 32MB file size limit to prevent memory exhaustion
- Handles Linux special files (procfs, sysfs) via fallback methods
- Uses realpath for secure path resolution

## Flow

1. Open file with appropriate mode
2. Check size limits
3. Read/write using streams or POSIX APIs
4. Handle special cases (proc files, FD-based access)
5. Close resources with proper error handling

## Integration

**Dependencies:**
- lib/misc (logging)
- lib/directory_ex (PathToRealPath)

**Used By:**
- System components needing file I/O
- Rust code (via cxx bridge)

**CMake:** Static library with C++ and optional Rust support
