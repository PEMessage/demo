# persistable_bundle_demo - Key-Value Persistence Demo

## Responsibility

Demonstrates the `PersistableBundle` class for storing and persisting key-value data. Shows type-safe storage of primitives, strings, vectors, and nested bundles with serialization to/from Parcel.

## Design

### X-Macro Pattern for Type Safety
```cpp
// Type list defined once, used for multiple method declarations
#define PERSISTABLE_BUNDLE_TYPES(X) \
    X(bool, Bool) \
    X(int32_t, Int32) \
    X(int64_t, Int64) \
    X(std::string, String) \
    // ... more types

// Generates PutBool(), PutInt32(), PutString(), etc.
#define DECLARE_PUT_METHOD(type, name) \
    void Put##name(const std::string &key, const type &value);
```

### Marshalling Pattern
```cpp
// To Parcel
Parcel parcel;
bundle.Marshalling(parcel);

// From Parcel
PersistableBundle* restored = PersistableBundle::Unmarshalling(parcel);
```

### Key Operations
```cpp
PersistableBundle bundle;

// Put values
bundle.PutString("name", "OpenHarmony");
bundle.PutInt32("version", 4);
bundle.PutBool("enabled", true);
bundle.PutStringVector("message", {"Hello", "World"});

// Get values
std::string name;
if (bundle.GetString("name", name)) {
    // Use name
}

// Query
auto keys = bundle.GetAllKeys();
size_t size = bundle.Size();
bundle.Erase("enabled");
```

## Flow

```
main.cpp
├── Create empty PersistableBundle
├── PutString("name", "OpenHarmony")
│   └── Show hex dump of serialized data
├── PutInt32("version", 4)
├── PutBool("enabled", true)
├── PutStringVector("message", ["Hello", "World"])
├── Print bundle info
├── Serialize to Parcel (Marshalling)
├── Write additional data (77) after bundle
├── Copy/Restore parcel
├── Unmarshal to new PersistableBundle
├── Print restored bundle info
├── Erase "enabled" key
├── Print final state
└── Cleanup
```

## Integration

### Dependencies
- **parcel**: Serialization framework
- **misc**: Miscellaneous utilities
- **string_ex**: String extensions

### Files
- `main.cpp`: Demo entry point and test cases
- `persistable_bundle.h`: Bundle class definition
- `persistable_bundle.cpp`: Implementation

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp persistable_bundle.cpp)
target_include_directories(app PUBLIC .)
auto_target_link_libraries(app PRIVATE parcel misc string_ex)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
=== Create PersistableBundle ===
-- Create
[hex dump of empty parcel]

-- Write 'String'
[hex dump with string data]

-- Write 'Int32'
[hex dump with int data]

Bundle size: 4
name: OpenHarmony
version: 4
enabled: true
message:
 | Hello
 | World

===  After restore ===
restoreValue: 77
[restored bundle info]
```

### Supported Types

| Type | Method Prefix |
|------|---------------|
| bool | PutBool / GetBool |
| int32_t | PutInt32 / GetInt32 |
| int64_t | PutInt64 / GetInt64 |
| uint8_t | PutUint8 / GetUint8 |
| uint16_t | PutUint16 / GetUint16 |
| uint32_t | PutUint32 / GetUint32 |
| uint64_t | PutUint64 / GetUint64 |
| float | PutFloat / GetFloat |
| double | PutDouble / GetDouble |
| std::string | PutString / GetString |
| std::u16string | PutString16 / GetString16 |
| std::vector<T> | Put<Type>Vector / Get<Type>Vector |
| PersistableBundle | PutPersistableBundle / GetPersistableBundle |

### Key APIs

| API | Purpose |
|-----|---------|
| `Put<Type>(key, value)` | Store typed value |
| `Get<Type>(key, &value)` | Retrieve typed value (returns bool) |
| `Marshalling(parcel)` | Serialize to Parcel |
| `Unmarshalling(parcel)` | Deserialize from Parcel |
| `GetAllKeys()` | Get all key names |
| `Size()` | Get number of entries |
| `Erase(key)` | Remove entry |
| `Clear()` | Remove all entries |
| `Empty()` | Check if empty |

## Notes

- Uses X-macros to generate type-specific methods from a single type list
- Serialization format is compatible with OpenHarmony's Parcel
- Get methods return bool indicating if key exists and type matches
- Vectors are stored with type information for safety
- Nested bundles are fully supported
