# lib/directory_ex/

## Responsibility

Provides directory and filesystem path operations including recursive traversal, creation/deletion, permission management, and path manipulation utilities.

## Design

**Key Function Groups:**

1. **Path Manipulation:**
   - `GetCurrentProcFullFileName()` - Get executable path via /proc/self/exe
   - `GetCurrentProcPath()` - Get directory containing executable
   - `ExtractFilePath()`, `ExtractFileName()`, `ExtractFileExt()` - Path parsing
   - `ExcludeTrailingPathDelimiter()`, `IncludeTrailingPathDelimiter()` - Path normalization

2. **Directory Operations:**
   - `GetDirFiles()` - Recursive file listing using stack-based traversal
   - `ForceCreateDirectory()` - Recursive mkdir (mkdir -p)
   - `ForceRemoveDirectory()` - Recursive directory removal with stack
   - `ForceRemoveDirectoryBMS()` - BMS-specific variant
   - `IsEmptyFolder()` - Check if directory has no files
   - `GetFolderSize()` - Calculate total size recursively

3. **File Operations:**
   - `RemoveFile()` - Safe file deletion
   - `ChangeModeFile()`, `ChangeModeDirectory()` - chmod recursively

4. **Path Resolution:**
   - `PathToRealPath()` - Resolve to absolute path with symlink resolution

**Security Features:**
- Uses `O_NOFOLLOW` to prevent symlink attacks
- Uses `openat`/`fdopendir` for safe directory traversal
- Stack-based traversal avoids recursion limits

## Flow

1. Path operations use string manipulation for normalization
2. Directory traversal uses stack to track nested directories
3. File operations use directory FDs for safe relative access
4. All operations include proper error handling and logging

## Integration

**Dependencies:**
- lib/misc (logging macros)
- lib/libsec (secureC functions)

**Used By:**
- lib/file_ex (PathToRealPath)
- BMS (Bundle Manager Service)
- Various system services

**CMake:** Static library with C++ and optional Rust support
