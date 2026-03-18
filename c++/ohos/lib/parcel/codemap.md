# lib/parcel/

Binary data serialization for Inter-Process Communication (IPC) in OpenHarmony.

The Parcel library provides a message container that serializes and deserializes data for cross-process data exchange. It serves as the foundation for OpenHarmony's IPC/RPC mechanism, enabling structured data transfer between processes through a platform-independent binary format.

---

## Responsibility

**Core Functions:**
- Serialize primitive types (bool, int, float, double, etc.) into binary format
- Serialize complex objects via the `Parcelable` interface
- Handle remote object references for IPC transactions
- Manage memory allocation and capacity expansion automatically
- Support both aligned and unaligned data access patterns

**Use Cases:**
- IPC/RPC data marshalling between processes
- Binder driver communication
- Service interface method parameter serialization
- Return value transmission across process boundaries

---

## Design

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        Parcel                               │
├─────────────────────────────────────────────────────────────┤
│  Data Buffer          │  Object Offsets Array               │
│  (uint8_t* data_)     │  (binder_size_t* objectOffsets_)      │
├─────────────────────────────────────────────────────────────┤
│  Write Operations     │  Read Operations                      │
│  - Primitives         │  - Primitives                         │
│  - Strings            │  - Strings                            │
│  - Buffers            │  - Buffers                            │
│  - Vectors            │  - Vectors                            │
│  - Parcelable objects │  - Parcelable objects                 │
│  - Remote objects     │  - Remote objects                     │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

#### 1. Parcel Class (parcel.h:176)
Main message container with dual-cursor design:
- **writeCursor_**: Position for next write operation
- **readCursor_**: Position for next read operation
- **data_**: Pointer to raw byte buffer
- **objectOffsets_**: Array tracking positions of embedded objects

**Capacity Management:**
- Default capacity: 200KB (`DEFAULT_CPACITY`)
- Threshold: 4KB (`CAPACITY_THRESHOLD`)
- Below threshold: Double the capacity
- Above threshold: Increase by 4KB steps
- Maximum capacity: Configurable via `SetMaxCapacity()`

#### 2. Parcelable Abstract Class (parcel.h:44)
Interface for serializable objects:
```cpp
class Parcelable : public virtual RefBase {
    virtual bool Marshalling(Parcel &parcel) const = 0;
    // static T* Unmarshalling(Parcel &parcel); // Required in subclass
};
```

**Behavior Flags:**
- `IPC` (0x01): Object usable in IPC
- `RPC` (0x02): Object usable in RPC
- `HOLD_OBJECT` (0x10): Keep object alive during transaction

#### 3. Allocator Interface (parcel.h:125)
Pluggable memory management:
```cpp
class Allocator {
    virtual void* Realloc(void *data, size_t newSize) = 0;
    virtual void* Alloc(size_t size) = 0;
    virtual void Dealloc(void *data) = 0;
};
```
- `DefaultAllocator`: Uses malloc/free/realloc
- Custom allocators can be injected for special memory requirements

#### 4. Flat Binder Object (flat_obj.h:47)
Structure for IPC object references:
```cpp
struct parcel_flat_binder_object {
    struct parcel_binder_object_header hdr;  // Type: BINDER_TYPE_HANDLE/BINDER_TYPE_FD
    __u32 flags;
    union {
        binder_uintptr_t binder;  // Local binder address
        __u32 handle;             // Remote handle
    };
    binder_uintptr_t cookie;
};
```

### Data Format

#### Memory Layout
```
┌────────────────────────────────────────────────────────────┐
│                    Parcel Data Buffer                       │
├────────────────────────────────────────────────────────────┤
│  [Primitive Data] [String Data] [Object Data] [Padding]    │
│                                                            │
│  Object Offsets Array:                                     │
│  [offset1, offset2, offset3, ...]  → Points to object      │
│                                     locations in buffer    │
└────────────────────────────────────────────────────────────┘
```

#### Alignment Rules
- Default: 4-byte alignment for all data types
- Small types (bool, int8, int16, uint8, uint16): Written as 32-bit for alignment
- `*Unaligned` methods: Write raw sizes without padding
- Padding bytes are zeroed for security

#### String Format
```
[int32 length][string data][null terminator][padding]
```
- Length prefix: 4 bytes (negative = null string)
- Null terminator included
- Padded to 4-byte boundary

#### Parcelable Object Format
```
[int32 meta]  // 0 = null, 1 = valid object
[object data] // Result of Marshalling()
```

---

## Flow

### Data Marshalling (Serialization)

```
┌──────────────┐     ┌──────────────────┐     ┌──────────────┐
│   Object     │────▶│  Marshalling()   │────▶│    Parcel    │
│  (App Data)  │     │  (User Defined)  │     │ (Byte Stream)│
└──────────────┘     └──────────────────┘     └──────────────┘
                                                        │
                                                        ▼
                                              ┌──────────────────┐
                                              │  Write Primitives │
                                              │  Write Strings    │
                                              │  Write Objects    │
                                              └──────────────────┘
```

**Typical Flow:**
1. Application calls `WriteParcelable(object)`
2. If object is null → write `0` as metadata
3. If object is remote → write `1` + call `WriteRemoteObject()`
4. If object is local → write `1` + call `object->Marshalling(parcel)`
5. `Marshalling()` writes individual fields using `WriteInt32()`, `WriteString()`, etc.

### Data Unmarshalling (Deserialization)

```
┌──────────────┐     ┌──────────────────┐     ┌──────────────┐
│    Parcel    │────▶│ Unmarshalling()  │────▶│   Object     │
│ (Byte Stream)│     │  (User Defined)  │     │  (App Data)  │
└──────────────┘     └──────────────────┘     └──────────────┘
        │
        ▼
┌──────────────────┐
│  Read Primitives  │
│  Read Strings     │
│  Read Objects     │
└──────────────────┘
```

**Typical Flow:**
1. Application calls `ReadParcelable<T>()`
2. Read metadata `int32` (0 = return null)
3. Call `T::Unmarshalling(parcel)` (static factory method)
4. `Unmarshalling()` reads individual fields using `ReadInt32()`, `ReadString()`, etc.
5. Return constructed object

### Vector Serialization

**Write Flow:**
1. Write `int32` vector size
2. Loop through elements, calling appropriate write method
3. Add padding to align to 4 bytes

**Read Flow:**
1. Read `int32` vector size
2. Validate size against available bytes
3. Resize vector
4. Loop through elements, calling appropriate read method
5. Skip padding bytes

### Remote Object Handling

```
WriteRemoteObject(object):
    1. Ensure object offsets capacity
    2. Record current write position
    3. Call object->Marshalling(parcel)
    4. Store offset in objectOffsets_ array
    5. If HOLD_OBJECT: add to objectHolder_ vector
```

```
ReadObject<T>():
    1. Call CheckOffsets() - verify read position is valid object location
    2. Call T::Unmarshalling(parcel)
    3. Return smart pointer
```

---

## Integration

### Dependencies

**Build Dependencies (CMakeLists.txt:8):**
```cmake
target_link_libraries(parcel PUBLIC misc refbase libsec nocopyable)
```

| Dependency | Purpose |
|------------|---------|
| `misc` | `flat_obj.h` - Binder object structures |
| `refbase` | `sptr<T>`, `RefBase` - Smart pointer support |
| `libsec` | `securec.h` - Safe string/memory operations |
| `nocopyable` | `DISALLOW_COPY_AND_MOVE` macro |

**Header Includes:**
- `<string>`, `<vector>` - STL containers
- `"nocopyable.h"` - Copy/move prevention macros
- `"refbase.h"` - Reference counting for objects
- `"flat_obj.h"` - Binder object structures

### Usage Patterns

#### 1. Basic Primitive Write/Read
```cpp
Parcel parcel;
parcel.WriteInt32(42);
parcel.WriteString("hello");

int32_t num = parcel.ReadInt32();
std::string str = parcel.ReadString();
```

#### 2. Custom Parcelable Object
```cpp
class MyData : public Parcelable {
public:
    bool Marshalling(Parcel &parcel) const override {
        parcel.WriteInt32(id_);
        parcel.WriteString(name_);
        return true;
    }
    
    static MyData* Unmarshalling(Parcel &parcel) {
        MyData* data = new MyData();
        data->id_ = parcel.ReadInt32();
        data->name_ = parcel.ReadString();
        return data;
    }
private:
    int32_t id_;
    std::string name_;
};
```

#### 3. IPC Transaction
```cpp
// Sender
Parcel data;
data.WriteRemoteObject(callback_);
data.WriteInt32(requestCode);

// Receiver (in another process)
sptr<ICallback> callback = data.ReadObject<ICallback>();
int32_t code = data.ReadInt32();
```

#### 4. Vector Operations
```cpp
std::vector<int32_t> nums = {1, 2, 3};
parcel.WriteInt32Vector(nums);

std::vector<int32_t> result;
parcel.ReadInt32Vector(&result);
```

### Thread Safety

The Parcel class itself is **not thread-safe**. Concurrent access requires external synchronization:
- Single writer, single reader: No synchronization needed
- Multiple threads: Wrap with mutex or use separate Parcel instances

### Security Considerations

1. **PARCEL_OBJECT_CHECK**: Optional compile-time validation to prevent reading object data as primitive data
2. **Size Validation**: All read operations validate bounds before accessing memory
3. **Null Terminators**: String reads verify null termination
4. **Capacity Limits**: Maximum capacity prevents unbounded memory growth

---

## File Reference

| File | Lines | Description |
|------|-------|-------------|
| `parcel.h` | 902 | Public API declarations, templates, inline methods |
| `parcel.cpp` | 1633 | Implementation of Parcel and helper classes |
| `CMakeLists.txt` | 8 | Build configuration with dependencies |

---

*Copyright (c) 2021 Huawei Device Co., Ltd. Licensed under Apache 2.0*
