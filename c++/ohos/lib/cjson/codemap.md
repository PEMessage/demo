# lib/cjson/

Lightweight JSON parsing and generation library (cJSON v1.7.18 by Dave Gamble).

## Responsibility

Provides JSON parsing (text → C structures) and generation (C structures → text) functionality:
- **Parsing**: Convert JSON text strings into navigable C data structures
- **Generation**: Serialize C data structures back to JSON text (formatted or compact)
- **Manipulation**: Create, modify, query, and delete JSON objects programmatically
- **Memory Safety**: Custom memory hook support for embedded/resource-constrained environments

## Design

### Core Data Structure: `cJSON`

The library centers around a single linked-list node structure (`cJSON.h:120-137`):

```c
typedef struct cJSON {
    struct cJSON *next;   // Sibling pointer (for arrays/objects)
    struct cJSON *prev;   // Previous sibling (circular list)
    struct cJSON *child;  // First child (for arrays/objects)
    int type;             // JSON type + flags (cJSON_Invalid, cJSON_String, etc.)
    char *valuestring;    // String value (for string/raw types)
    int valueint;         // Integer value (DEPRECATED)
    double valuedouble;   // Numeric value
    char *string;         // Key name (for object children)
} cJSON;
```

**Type System** (`cJSON.h:78-86`):
- `cJSON_Invalid`, `cJSON_False`, `cJSON_True`, `cJSON_NULL`
- `cJSON_Number`, `cJSON_String`, `cJSON_Array`, `cJSON_Object`, `cJSON_Raw`
- Flags: `cJSON_IsReference` (don't free), `cJSON_StringIsConst` (const key)

### API Architecture

**Parse API** (text → cJSON):
- `cJSON_Parse()` - Parse null-terminated string
- `cJSON_ParseWithLength()` - Parse with explicit length
- `cJSON_ParseWithOpts()` - Extended options (error pointer, null-termination check)

**Print API** (cJSON → text):
- `cJSON_Print()` - Formatted output
- `cJSON_PrintUnformatted()` - Compact output
- `cJSON_PrintBuffered()` - Pre-allocated buffer hint
- `cJSON_PrintPreallocated()` - User-provided buffer

**Create API**:
- `cJSON_CreateNull/True/False/Bool/Number/String/Raw/Array/Object()` - Type constructors
- `cJSON_Create*Array()` - Array from C arrays (int, float, double, string)
- `cJSON_Add*ToObject()` - Create + add convenience functions

**Query API**:
- `cJSON_GetArraySize/Item()` - Array access
- `cJSON_GetObjectItem/CaseSensitive()` - Object key lookup
- `cJSON_Is*()` - Type checking predicates
- `cJSON_GetString/NumberValue()` - Value extraction

**Modify API**:
- `cJSON_AddItemToArray/Object()` - Append/insert items
- `cJSON_ReplaceItem*()` - Replace existing items
- `cJSON_DeleteItem*()` - Remove and free items
- `cJSON_DetachItem*()` - Remove without freeing

**Memory Management**:
- `cJSON_Delete()` - Recursive free of tree
- `cJSON_InitHooks()` - Custom malloc/free/realloc hooks

### Parser Design (`cJSON.c`)

**State Management**:
- `parse_buffer` struct tracks input content, position, depth (nesting protection)
- `CJSON_NESTING_LIMIT` (default 1000) prevents stack overflow from deeply nested JSON
- Global error tracking via `global_error` struct

**Recursive Descent Parsing**:
- `parse_value()` - Entry point, dispatches by first character
- `parse_number()` - Numeric parsing with locale-aware decimal point
- `parse_string()` - String with escape sequence and UTF-16→UTF-8 conversion
- `parse_array()` - '['...']' with comma-separated values
- `parse_object()` - '{'...'}' with "key":value pairs

**UTF-8 Handling**:
- `parse_hex4()` - Hex digit parsing
- `utf16_literal_to_utf8()` - Surrogate pair support for BMP+ characters

### Printer Design (`cJSON.c`)

**Buffer Management**:
- `printbuffer` struct manages dynamically growing output buffer
- `ensure()` - Buffer reallocation with size doubling strategy
- Supports both allocated and preallocated buffer modes

**Recursive Output**:
- `print_value()` - Type dispatch for serialization
- `print_number()` - Precision-aware number formatting (15→17 digits fallback)
- `print_string_ptr()` - Escape sequence generation
- `print_array/object()` - Container formatting with indentation support

## Flow

### Parsing Flow

```
Input JSON String
      ↓
cJSON_ParseWithLengthOpts()
      ↓
Initialize parse_buffer (content, length, offset=0, depth=0)
      ↓
cJSON_New_Item() → Allocate root cJSON node
      ↓
parse_value() ───────────────────────────────────────────┐
      │                                                  │
      ├── "null"  → type=cJSON_NULL                      │
      ├── "true"  → type=cJSON_True, valueint=1          │
      ├── "false" → type=cJSON_False                     │
      ├── '"'     → parse_string() → valuestring         │
      ├── '-',0-9 → parse_number() → valuedouble/valueint│
      ├── '['     → parse_array() ───────────────────────┤
      │               Allocate child nodes recursively   │
      └── '{'     → parse_object() ──────────────────────┤
                      "key": value pairs                 │
                                                         │
      ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←┘
      ↓
Return cJSON* tree root
      ↓
User navigates/modifies via Get*/Add*/Replace* APIs
      ↓
cJSON_Delete() → Recursive cleanup
```

### Generation Flow

```
cJSON* tree root
      ↓
cJSON_Print() / cJSON_PrintUnformatted()
      ↓
print() → Initialize printbuffer (256 bytes default)
      ↓
print_value() ───────────────────────────────────────────┐
      │                                                  │
      ├── cJSON_NULL    → "null"                         │
      ├── cJSON_False   → "false"                        │
      ├── cJSON_True    → "true"                         │
      ├── cJSON_Number  → print_number()                 │
      ├── cJSON_String  → print_string() → escapes       │
      ├── cJSON_Array   → print_array() ─────────────────┤
      └── cJSON_Object  → print_object() ────────────────┤
                                                         │
      ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←┘
      ↓
update_offset() → Finalize buffer
      ↓
Return char* JSON string (caller must free)
```

### Object Creation Flow

```
Option A: Parse existing JSON
    cJSON_Parse(json_string) → Complete tree

Option B: Build programmatically
    cJSON_CreateObject() → Root
           ↓
    cJSON_CreateString/Number/...() → Values
           ↓
    cJSON_AddItemToObject(root, "key", value)
           ↓
    cJSON_Print(root) → JSON string
           ↓
    cJSON_Delete(root) → Cleanup
```

## Integration

### Dependencies

**Build Dependencies** (`CMakeLists.txt`):
- CMake ≥3.0
- Standard C library (stdlib, string, math, stdio, ctype, float, limits)
- Optional: `locale.h` (ENABLE_LOCALES for decimal point handling)

**Link Dependencies**:
- Math library (`-lm` on non-Windows)

**Fetch Content Integration** (local `CMakeLists.txt`):
```cmake
set(CJSON_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")
if(NOT EXISTS "${CJSON_SOURCE_DIR}")
    FetchContent_Declare(
        cjson
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG v1.7.18
        SOURCE_DIR "${CJSON_SOURCE_DIR}"
    )
    FetchContent_MakeAvailable(cjson)
else()
    add_subdirectory(${CJSON_SOURCE_DIR})
endif()
target_include_directories(cjson PUBLIC code)
```

### Usage Patterns

**Basic Parse/Print**:
```c
const char *json = "{\"name\":\"test\",\"value\":42}";
cJSON *root = cJSON_Parse(json);
cJSON *name = cJSON_GetObjectItem(root, "name");
printf("Name: %s\n", name->valuestring);
char *output = cJSON_Print(root);
cJSON_Delete(root);
free(output);
```

**Building JSON**:
```c
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "name", "test");
cJSON_AddNumberToObject(root, "value", 42);
char *json = cJSON_PrintUnformatted(root);
cJSON_Delete(root);
```

**Custom Memory Hooks**:
```c
cJSON_Hooks hooks = { custom_malloc, custom_free };
cJSON_InitHooks(&hooks);
```

### Configuration Options

- `CJSON_NESTING_LIMIT` - Max parse depth (default: 1000)
- `ENABLE_LOCALES` - Locale-aware number parsing
- `CJSON_HIDE_SYMBOLS` / `CJSON_EXPORT_SYMBOLS` - Symbol visibility

### Security Considerations

- Always check `cJSON_Parse()` return value for NULL (parse error)
- Use `cJSON_GetErrorPtr()` to locate parse failures
- All `Print` outputs must be freed by caller
- Reference counting via `cJSON_IsReference` flag prevents double-free
- Nesting limit prevents stack overflow from malicious input
