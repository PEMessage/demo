# refbase_demo - Reference Counting Demo

## Responsibility

Demonstrates OpenHarmony's reference counting mechanism using `RefBase`, `sptr<>` (strong pointer), and `wptr<>` (weak pointer). Shows object lifecycle management, reference counting behavior, and common pitfalls.

## Design

### RefBase Usage
```cpp
struct A: RefBase {
    A() { printf("|  Create class A: %p\n", this); }
    ~A() { printf("|  ~ class A: %p\n", this); }
};
```

### sptr Patterns
```cpp
// Creation using MakeSptr (recommended)
sptr<A> pa = sptr<A>::MakeSptr();

// Copy (increments ref count)
sptr<A> pb = pa;
cout << "PB SptrRefCount: " << pb->GetSptrRefCount() << endl;  // 2

// Assignment (handles ref counting)
sptr<A> pc = pa;
pc = pb;  // pa ref--, pb ref++
```

## Flow

```
test_normal()
├── sptr<A> pa = MakeSptr()    // RefCount = 1
├── sptr<A> pb = pa            // RefCount = 2
├── Print ref counts
├── pb = NULL                  // RefCount = 1
└── return                     // RefCount = 0, destructor called

test_swap()
├── sptr<A> pa = MakeSptr()    // Object A, RefCount = 1
├── sptr<A> pb = MakeSptr()    // Object B, RefCount = 1
├── sptr<A> pc = pa            // Object A, RefCount = 2
├── pc = pb                    // Object A Ref--, Object B Ref++
└── return                     // All destroyed properly
```

## Integration

### Dependencies
- **refbase+debug_all**: Reference counting with debug output
- **hilog**: Logging support

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE refbase+debug_all hilog)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
--------------------
test_normal
--------------------
sptr<A> pa = sptr<A>::MakeSptr();
|  Create class A: 0x...
sptr<A> pb = pa;
PB SptrRefCount: 2
PB WptrRefCount: 1
pb = NULL;
return;
|  ~ class A: 0x...
```

### Key APIs

| API | Purpose |
|-----|---------|
| `RefBase` | Base class for reference-counted objects |
| `sptr<T>` | Strong pointer (increments strong ref count) |
| `wptr<T>` | Weak pointer (doesn't prevent destruction) |
| `MakeSptr()` | Factory method for safe creation |
| `GetSptrRefCount()` | Get strong reference count |
| `GetWptrRefCount()` | Get weak reference count |

## Important Warnings

### Never Create RefBase Objects on Stack
```cpp
void test_create_in_stack() {
    A a;                    // DON'T DO THIS!
    {
        sptr<A> pa = &a;    // pa manages stack object
    }                       // pa destroyed, deletes stack object!
}                           // DOUBLE FREE when a goes out of scope!
```

### Best Practices
1. Always use `MakeSptr()` for creation
2. Use `sptr<>` for ownership, `wptr<>` for non-owning references
3. Never create RefBase-derived objects on the stack
4. The `+debug_all` variant provides detailed lifecycle logging

## Notes

- `refbase+debug_all` includes additional debug output
- Reference counting is thread-safe
- Objects are destroyed when strong ref count reaches 0
- Weak pointers can detect if object has been destroyed
