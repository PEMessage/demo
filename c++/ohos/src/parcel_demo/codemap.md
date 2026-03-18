# parcel_demo - Basic Parcel Serialization Demo

## Responsibility

Demonstrates the core `Parcel` class for serializing and deserializing data. Shows how primitives, strings, vectors, and custom `Parcelable` objects are marshalled to and unmarshalled from a flat buffer.

## Design

### Parcelable Interface
```cpp
class MyParcelable : public Parcelable {
public:
    // Serialize to parcel
    bool Marshalling(Parcel &parcel) const override {
        return parcel.WriteInt32(value_);
    }
    
    // Deserialize from parcel (static factory)
    static MyParcelable *Unmarshalling(Parcel &parcel) {
        int32_t value;
        if (parcel.ReadInt32(value)) {
            return new MyParcelable(value);
        }
        return nullptr;
    }
};
```

### Data Alignment
```cpp
// Parcel automatically handles alignment
parcel.WriteInt32(42);      // 4 bytes, aligned
parcel.WriteString("Hi");   // size(4) + data + padding to 4-byte boundary
```

### Hex Dump Visualization
```cpp
#define DUMP_PARCEL(parcel) xxd_color2(parcel.GetDataSize(), \
                          (unsigned char*)parcel.GetData())
```
Shows memory layout with color-coded bytes.

## Flow

```
main.cpp
├── Create empty Parcel
├── Write Int32: 42
│   └── [hex dump: 2a 00 00 00]
├── Write Float: 3.14f
│   └── [hex dump with float bytes]
├── Write String: "Hello, Parcel!"
│   └── [hex: size(14) + data + padding(2)]
├── Write String: "Hello, Parcel!xx"
│   └── [hex: size(16) + data + padding(4)]
├── Write vector<int32_t>: {1, 2, 3, 4, 5}
│   └── [hex: size(5) + elements]
├── Write vector<string>: {"s1", "s2"}
├── Write MyParcelable: 100
├── RewindRead(0) to reset read position
├── Read back all values in order
│   └── Verify they match original values
└── Cleanup
```

## Integration

### Dependencies
- **parcel**: `/home/zhuojw/demo_new/c++/ohos/lib/parcel/`

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE parcel)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
------------------------------------
Empty parcel
[empty hex dump]
------------------------------------


------------------------------------
Write Int32: 42
2a 00 00 00                                         |*...|
------------------------------------


------------------------------------
Write String: Hello, Parcel!
| size(4b) + data(14) + padding(2b)
------------------------------------

Read int: 42
Read float: 3.14
Read string: Hello, Parcel!
...
```

### Supported Types

| Type | Write API | Read API |
|------|-----------|----------|
| int32_t | `WriteInt32(val)` | `ReadInt32(&val)` |
| float | `WriteFloat(val)` | `ReadFloat(&val)` |
| std::string | `WriteString(str)` | `ReadString()` |
| std::vector<int32_t> | `WriteInt32Vector(vec)` | `ReadInt32Vector(&vec)` |
| std::vector<std::string> | `WriteStringVector(vec)` | `ReadStringVector(&vec)` |
| Parcelable* | `WriteParcelable(obj)` | Custom Unmarshalling |

### Key APIs

| API | Purpose |
|-----|---------|
| `WriteInt32(val)` | Write 32-bit integer |
| `ReadInt32(&val)` | Read 32-bit integer (returns bool) |
| `WriteString(str)` | Write length-prefixed string |
| `ReadString()` | Read string (returns std::string) |
| `RewindRead(0)` | Reset read position to start |
| `GetData()` | Get raw data pointer |
| `GetDataSize()` | Get data size in bytes |
| `Marshalling()` | Virtual: serialize to parcel |
| `Unmarshalling()` | Static: deserialize from parcel |

## Notes

- String format: `size(4 bytes) + data + padding` (padded to 4-byte boundary)
- Vector format: `size(4 bytes) + elements`
- Custom objects must implement `Parcelable` interface
- Read position advances automatically; use `RewindRead()` to reset
- The demo visualizes memory layout using `xxd_color2()`
