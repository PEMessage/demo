# message_parcel_demo - IPC Message Parcel Demo

## Responsibility

Extends `Parcel` with IPC-specific features. Demonstrates `MessageParcel` for inter-process communication, including interface tokens for security, exception handling, and UTF-16 string support.

## Design

### IPC Security Features
```cpp
MessageParcel parcel;

// Write interface token for security validation
parcel.WriteInterfaceToken(u"test.interface.token");

// Mark no exception occurred
parcel.WriteNoException();

// Later, check for remote exceptions
int32_t exception = parcel.ReadException();
```

### UTF-16 Strings for IPC
```cpp
// UTF-16 strings (std::u16string) for cross-platform compatibility
parcel.WriteString16(u"Hello, IPC_SINGLE!");
std::u16string str = parcel.ReadString16();

// Convert to std::string for display
std::string display(str.begin(), str.end());
```

## Flow

```
main.cpp
├── Create MessageParcel
├── Write Int32: 42
├── Write String16: u"Hello, IPC_SINGLE!"
├── Write Bool: true
├── Write InterfaceToken: u"test.interface.token"
│   └── Security token for interface validation
├── Write NoException
│   └── Signals no error occurred
├── RewindRead(0) to reset position
├── Read Int32
├── Read String16 (convert to std::string for display)
├── Read Bool
├── Read InterfaceToken
│   └── Can be validated against expected token
├── Read Exception (should be 0 for no exception)
├── Print all values
└── Return 0
```

## Integration

### Dependencies
- **ipc**: `/home/zhuojw/demo_new/c++/ohos/lib/ipc/`
- **ashmem**: Required by ipc library

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### CMakeLists.txt
```cmake
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE ipc ashmem)
```

## Usage

### Running the Demo
```bash
./app
```

**Expected Output:**
```
Value: 42
String: Hello, IPC_SINGLE!
Flag: true
Token: test.interface.token
Exception: 0
```

### Key APIs

| API | Purpose |
|-----|---------|
| `WriteInt32(val)` | Write 32-bit integer |
| `WriteString16(str)` | Write UTF-16 string |
| `ReadString16()` | Read UTF-16 string |
| `WriteBool(val)` | Write boolean |
| `ReadBool()` | Read boolean |
| `WriteInterfaceToken(token)` | Write security token |
| `ReadInterfaceToken()` | Read security token |
| `WriteNoException()` | Mark success (no error) |
| `ReadException()` | Get exception code (0 = OK) |
| `RewindRead(pos)` | Reset read position |

### IPC Data Types

```cpp
// Basic types
int32_t, int64_t, uint32_t, uint64_t
float, double
bool

// IPC-specific types
std::u16string              // UTF-16 strings
std::vector<std::u16string> // UTF-16 string vectors

// Security
std::u16string token        // Interface token for validation
int32_t exception           // Exception code (0 = success)
```

### Interface Token Pattern
```cpp
// Sender writes token
parcel.WriteInterfaceToken(u"com.example.IService");

// Receiver validates
token = parcel.ReadInterfaceToken();
if (token != expectedToken) {
    reply.WriteException(ERR_INVALID_DATA);
    return;
}
```

## Notes

- `MessageParcel` extends `Parcel` with IPC-specific features
- UTF-16 strings ensure cross-platform compatibility
- Interface tokens provide security by validating expected interface
- Exceptions allow remote errors to be propagated
- Use `WriteNoException()` when operation succeeds
- Check `ReadException()` on receiving side
