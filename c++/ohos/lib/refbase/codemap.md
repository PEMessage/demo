# RefBase Codemap

## Responsibility

**RefBase** is the OpenHarmony reference counting base library providing automatic memory management for C++ objects. It implements a smart pointer system similar to Android's RefBase, enabling safe shared ownership and lifecycle management of objects.

### Core Components

| Component | File | Purpose |
|-----------|------|---------|
| `RefBase` | refbase.h | Base class for reference-counted objects |
| `RefCounter` | refbase.h/cpp | Internal reference counter managing strong/weak refs |
| `WeakRefCounter` | refbase.h/cpp | Intermediate weak reference holder |
| `sptr<T>` | refbase.h | Strong smart pointer template (header-only) |
| `wptr<T>` | refbase.h | Weak smart pointer template (header-only) |

---

## Design

### RefBase Pattern

The library implements a **three-tier reference counting architecture**:

```
┌─────────────────────────────────────────────────────────────┐
│                        Object Layer                         │
│  ┌───────────────┐                                          │
│  │   RefBase     │  ← Base class for managed objects         │
│  │   (refs_)     │  ← Pointer to RefCounter                  │
│  └───────────────┘                                          │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     RefCounter Layer                        │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │ atomicStrong_ │  │  atomicWeak_  │  │ atomicRefCount_ │ │
│  │  (sptr count) │  │ (sptr+wptr)   │  │ (WeakRefCounter)│ │
│  └───────────────┘  └───────────────┘  └─────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ callback_ → RefPtrCallback() → delete RefBase object   │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                    WeakRefCounter Layer                     │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │  atomicWeak_  │  │  refCounter_  │  │    cookie_      │ │
│  │ (wptr count)  │  │  → RefCounter │  │ → RefBase obj   │ │
│  └───────────────┘  └───────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Strong vs Weak References

| Type | Class | Behavior | Use Case |
|------|-------|----------|----------|
| **Strong** | `sptr<T>` | Keeps object alive; object destroyed when last strong ref released | Regular ownership |
| **Weak** | `wptr<T>` | Doesn't keep object alive; can detect if object still exists | Breaking cycles, observers |

### Key Design Patterns

1. **Separated Reference Counter**: `RefCounter` exists independently from the managed object, allowing weak references to detect object destruction even after the object is gone.

2. **Initial Primary Value**: `INITIAL_PRIMARY_VALUE = 1 << 28` is used to distinguish between:
   - Object with strong references (count > 0)
   - Object never had strong references (count == INITIAL_PRIMARY_VALUE)
   - Object was destroyed (count == 0)

3. **Extended Lifetime**: Objects can call `ExtendObjectLifetime()` to stay alive with only weak references (useful for caches).

4. **Promotion**: Weak pointers can attempt to "promote" to strong pointers via `AttemptIncStrongRef()`.

---

## Flow

### Object Lifecycle

```
Creation Flow:
┌──────────┐    ┌──────────────┐    ┌─────────────┐    ┌─────────────┐
│ sptr<T>  │───→│ new T(...)   │───→│ RefBase()   │───→│ RefCounter  │
│ MakeSptr │    │              │    │             │    │ (refs_)     │
└──────────┘    └──────────────┘    └─────────────┘    └─────────────┘
                                                            │
                                                       IncRefCount()

Strong Reference Operations:
┌──────────────┐
│  IncStrong   │───→ atomicStrong_.fetch_add(1)
│  Ref(object) │     If first ref (== INITIAL_PRIMARY_VALUE):
└──────────────┘     └──→ OnFirstStrongRef()
                            │
┌──────────────┐              │
│  DecStrong   │              │
│  Ref(object) │              └──→ OnLastStrongRef()
└──────────────┘                    │
       │                       If !extendedLifetime:
       ▼                       └──→ callback_() → delete object
  atomicStrong_.fetch_sub(1)
  If last ref (== 1):
  └──→ DecWeakRefCount()

Weak Reference Operations:
┌──────────────┐
│ wptr<T> ctor │───→ CreateWeakRef() ──→ new WeakRefCounter
└──────────────┘                           │
                                      IncWeakRefCount()
                                           │
┌──────────────┐                             │
│  promote()   │                             ▼
│ (AttemptInc  │    ┌──────────────────────────────────────────┐
│  StrongRef)  │───→│  CAS loop on atomicStrong_               │
└──────────────┘    │  If success: return sptr<T>              │
                    │  If fail:    return nullptr sptr         │
                    └──────────────────────────────────────────┘
```

### Destruction Flow

**Normal Case (Non-Extended Lifetime):**
```
Last sptr destroyed
       │
       ▼
DecStrongRefCount() → curCount == 1
       │
       ▼
OnLastStrongRef() (virtual hook)
       │
       ▼
callback_() → RefPtrCallback() → delete this
       │
       ▼
~RefBase() → RemoveCallback() → DecRefCount()
       │
       ▼
~RefCounter() (when atomicRefCount_ reaches 0)
```

**Extended Lifetime Case:**
```
Last sptr destroyed (strongCount == 0)
       │
       ▼
Object NOT destroyed (extended lifetime)
       │
       ▼
Wait for last weak reference release
       │
       ▼
DecWeakRefCount() → curCount == 1
       │
       ▼
callback_() → delete object (if extended)
```

### Reference Counting Relationships

```
sptr count = atomicStrong_ / INITIAL_PRIMARY_VALUE indicator
     │
     ├─── 0: Object destroyed (or never existed)
     ├─── INITIAL_PRIMARY_VALUE: Object exists, no strong refs yet
     └─── > 0: Actual number of strong references

weak count (in RefCounter) = atomicWeak_
     │
     ├─── Number of sptr + number of WeakRefCounter objects
     └─── When reaches 0: May delete RefCounter (if no callbacks)

refCount = atomicRefCount_
     │
     ├─── Number of WeakRefCounter objects referencing this RefCounter
     └─── Plus 1 for the RefBase object itself
```

---

## Integration

### Dependencies

| Dependency | Purpose | Location |
|------------|---------|----------|
| `utils_log.h` | Logging macros (UTILS_LOGI, UTILS_LOGD, etc.) | External (c_utils) |
| `misc` | Build system library | CMake target |
| `<atomic>` | C++11 atomic operations | Standard library |
| `<functional>` | std::function for callbacks | Standard library |
| `<mutex>` | Debug tracker mutex | Standard library |

### Build Targets

From `CMakeLists.txt`:

| Target | Purpose | Debug Flags |
|--------|---------|-------------|
| `refbase` | Production static library | None |
| `refbase+debug_all` | Debug with tracking | DEBUG_REFBASE, PRINT_TRACK_AT_ONCE, TRACK_ALL |
| `refbase+debug` | Debug with tracking | Same as above |

### Usage Pattern

```cpp
// Inherit from RefBase
class MyClass : public OHOS::RefBase {
    // Implementation
};

// Create with sptr
OHOS::sptr<MyClass> obj = OHOS::sptr<MyClass>::MakeSptr(args...);

// Or (not recommended)
OHOS::sptr<MyClass> obj(new MyClass());

// Create weak reference
OHOS::wptr<MyClass> weak = obj;

// Promote weak to strong
OHOS::sptr<MyClass> strong = weak.promote();
if (strong != nullptr) {
    // Use strong reference
}
```

### Thread Safety

All reference count operations use `std::atomic` with appropriate memory ordering:
- `memory_order_relaxed` for most increments
- `memory_order_release` / `memory_order_acquire` for decrement/acquire patterns
- Mutex protection for debug tracking

---

## Key Implementation Details

### Memory Order Strategy

```cpp
// Increment: Relaxed (no synchronization needed)
atomicStrong_.fetch_add(1, std::memory_order_relaxed);

// Decrement: Release (publish changes before potential delete)
atomicStrong_.fetch_sub(1, std::memory_order_release);

// Before destruction: Acquire (ensure all writes are visible)
std::atomic_thread_fence(std::memory_order_acquire);
```

### Debug Features (DEBUG_REFBASE)

| Feature | Description |
|---------|-------------|
| `RefTracker` | Records stack trace of reference operations |
| `EnableTracker()` | Per-object tracking enablement |
| `PRINT_TRACK_AT_ONCE` | Log immediately vs. deferred backtrace |
| `TRACK_ALL` | Track all objects by default |

### Platform-Specific Features (OHOS_PLATFORM)

- `CanPromote` callback for IPC scenarios
- `CanPromote()` virtual method for subclasses to control promotion
