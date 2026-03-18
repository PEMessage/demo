# lib/unicode_ex/

## Responsibility

Provides UTF-8 and UTF-16 string conversion utilities. Handles proper encoding/decoding between Unicode formats including surrogate pairs for characters outside the Basic Multilingual Plane (BMP).

## Design

**Key Components:**
- `String8ToString16()` - Convert UTF-8 to UTF-16
- `String16ToString8()` - Convert UTF-16 to UTF-8
- `StrncpyStr16ToStr8()` - Low-level UTF-16 to UTF-8 copy with buffer
- `Utf16ToUtf8Length()` - Calculate required buffer size for conversion

**Encoding Support:**
- UTF-8: 1-4 bytes per character
- UTF-16: Handles surrogate pairs (U+10000 to U+10FFFF)
- Proper handling of reserved surrogate range (U+D800-U+DFFF)

**Pattern:** Low-level C functions with C++ wrapper std::string/u16string APIs

## Flow

1. Input string analyzed for character ranges
2. Surrogate pairs detected for non-BMP characters
3. Byte sequences calculated per UTF-8/UTF-16 rules
4. Safe conversion with length checking
5. Null-terminated output produced

## Integration

**Dependencies:**
- lib/misc (for UTILS_LOGD logging)

**Used By:**
- lib/string_ex (Str8ToStr16, Str16ToStr8)
- Any code needing Unicode conversions

**CMake:** Static library (`add_library(unicode_ex STATIC)`)
