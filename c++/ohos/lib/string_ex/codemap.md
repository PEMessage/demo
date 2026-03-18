# lib/string_ex/

## Responsibility

Provides extended string manipulation utilities beyond standard C++ library. Includes case conversion, trimming, splitting, substring extraction, and UTF-8/UTF-16 conversion.

## Design

**Key Function Groups:**

1. **Case Operations:**
   - `UpperStr()`, `LowerStr()` - Full string case conversion
   - `IsUpperStr()`, `IsLowerStr()` - Case validation

2. **Search/Replace:**
   - `ReplaceStr()` - Substring replacement
   - `IsSubStr()` - Substring existence check
   - `GetFirstSubStrBetween()`, `GetSubStrBetween()` - Extract content between delimiters

3. **Parsing:**
   - `SplitStr()` - Split by delimiter with options
   - `TrimStr()` - Remove leading/trailing characters
   - `StrToInt()` - Safe string to integer conversion
   - `DexToHexString()` - Decimal to hex conversion

4. **Validation:**
   - `IsNumericStr()`, `IsAlphaStr()` - Content type checks
   - `IsAsciiString()` - ASCII validation
   - `IsSameTextStr()` - Case-insensitive comparison

5. **Unicode:**
   - `Str8ToStr16()`, `Str16ToStr8()` - UTF conversion via unicode_ex
   - `GetUtf16ToUtf8Length()` - Calculate conversion buffer size

## Flow

String operations follow standard patterns:
1. Input validation (empty checks)
2. Algorithm application (transform, search, etc.)
3. Result construction and return

## Integration

**Dependencies:**
- lib/misc (logging macros)
- lib/unicode_ex (UTF conversions)

**Used By:**
- Various system components needing string manipulation

**CMake:** Static library linking to misc and unicode_ex
