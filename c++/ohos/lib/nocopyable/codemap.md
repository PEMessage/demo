# lib/nocopyable/

## Responsibility

Provides a base class and macros to disable copy and move semantics for C++ classes. This is a fundamental utility used throughout the OpenHarmony codebase to prevent accidental copying of objects that should be unique (like singletons, resource managers, etc.).

## Design

**Key Components:**
- `NoCopyable` class - Base class that disables copy/move via private deleted constructors
- `DISALLOW_COPY(className)` macro - Deletes copy constructor and copy assignment
- `DISALLOW_MOVE(className)` macro - Deletes move constructor and move assignment
- `DISALLOW_COPY_AND_MOVE(className)` macro - Combines both restrictions

**Pattern:** CRTP (Curiously Recurring Template Pattern) compatible base class that uses C++11 `= delete` to explicitly disable operations.

## Flow

1. Classes inherit from `NoCopyable` OR use the macros in their private section
2. Compiler prevents any copy/move operations at compile time
3. Results in clear error messages when accidental copies are attempted

## Integration

**Dependencies:** None (header-only, standalone)

**Used By:** 
- lib/misc (singleton.h uses NoCopyable)
- Most other libraries in the system that need non-copyable semantics

**CMake:** Interface library (`add_library(nocopyable INTERFACE)`)
