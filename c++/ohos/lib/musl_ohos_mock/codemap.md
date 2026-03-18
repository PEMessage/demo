# lib/musl_ohos_mock/

## Responsibility

Provides compatibility shims and mock implementations for musl libc extensions specific to OpenHarmony. Acts as a bridge between OHOS-specific APIs and standard interfaces.

## Design

**Key Components:**
- `unistd_ohos_mock.h` - Maps OHOS process ID functions to standard equivalents
  - `getprocpid` → `getpid`
  - `getproctid` → `gettid`
- `__mutex_base` - Empty file (placeholder for mutex implementations)
- `bits/ioctl.h` - Empty file (placeholder for ioctl constants)

**Pattern:** Header-only interface library with macro-based API remapping.

## Flow

1. Code includes OHOS-specific headers expecting `getprocpid()` etc.
2. This library remaps those calls to standard POSIX equivalents
3. Allows OHOS code to compile on standard Linux systems

## Integration

**Dependencies:** None (pure remapping layer)

**Used By:** 
- lib/misc (unistd_ohos_mock.h)

**CMake:** Interface library (`add_library(musl_ohos_mock INTERFACE)`)

**Note:** The empty files (`__mutex_base`, `bits/ioctl.h`) are placeholders for platform-specific implementations.
