// q-gcc: -std=c++20 --
#include <optional>
#include <iostream>
#include <type_traits>
#include <limits>
#include <utility>

// function under test
template <typename To, typename From>
auto try_narrow(const From& from) noexcept -> std::optional<To>
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) {
        return std::nullopt;          // conversion failed, data lost
    }
    return to;                        // success
}

// ---------------------------------------------------------------------------
// Implementation A: based on std::in_range<To> (C++20)
//
// std::in_range requires "standard integer types" only (short/int/long/long
// long and their unsigned variants) -- char and bool are NOT allowed. We make
// that contract explicit with a static_assert.
// ---------------------------------------------------------------------------
// "standard integer types" = std::is_integral minus the narrow/character types
// that std::in_range does not accept (char, signed/unsigned char, bool, and the
// wide/UTF char types).
template <typename T>
inline constexpr bool is_standard_integer =
    std::is_integral_v<T> &&
    !std::is_same_v<T, char> &&
    !std::is_same_v<T, signed char> &&
    !std::is_same_v<T, unsigned char> &&
    !std::is_same_v<T, bool> &&
    !std::is_same_v<T, wchar_t> &&
    !std::is_same_v<T, char8_t> &&
    !std::is_same_v<T, char16_t> &&
    !std::is_same_v<T, char32_t>;

template <typename To, typename From>
auto try_narrow_inrange(const From& from) noexcept -> std::optional<To>
{
    static_assert(is_standard_integer<To> && is_standard_integer<From>,
                  "try_narrow_inrange requires standard integer types "
                  "(char/bool/signed char/unsigned char are not allowed)");
    if (!std::in_range<To>(from)) {   // respects signedness and range
        return std::nullopt;
    }
    return static_cast<To>(from);
}

// ---------------------------------------------------------------------------
// Implementation B: the elegant round-trip check, with a MINIMAL fix
//
// The original bug: when From and To have different signedness, the wrapped
// value can round-trip back to `from`, hiding the loss. The minimal addition
// is to also require the sign to be preserved.
// ---------------------------------------------------------------------------
template <typename To, typename From>
auto try_narrow_roundtrip(const From& from) noexcept -> std::optional<To>
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) {
        return std::nullopt;
    }
    // minimal fix: detect signed/unsigned wrap-around that round-trips back
    if constexpr (std::is_unsigned_v<To> && std::is_signed_v<From>) {
        if (from < 0) {
            return std::nullopt;      // signed -> unsigned: sign lost
        }
    }
    if constexpr (std::is_signed_v<To> && std::is_unsigned_v<From>) {
        if (to < 0) {
            return std::nullopt;      // unsigned -> signed: overflow wrap
        }
    }
    return to;
}

// ---------------------------------------------------------------------------
// try_narrow_roundtrip_sfinae: pure SFINAE dispatch -- overloads selected by
// std::enable_if on the return type, NO if constexpr anywhere. The signedness
// combinations for integral types are split into separate overloads, and a
// final overload handles all non-integral (float) conversions.
// ---------------------------------------------------------------------------
// 1) both signed
template <typename To, typename From>
std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From> &&
                 std::is_signed_v<To> && std::is_signed_v<From>, std::optional<To>>
try_narrow_roundtrip_sfinae(const From& from) noexcept
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) return std::nullopt;
    return to;
}

// 2) both unsigned
template <typename To, typename From>
std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From> &&
                 std::is_unsigned_v<To> && std::is_unsigned_v<From>, std::optional<To>>
try_narrow_roundtrip_sfinae(const From& from) noexcept
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) return std::nullopt;
    return to;
}

// 3) To unsigned, From signed -> sign lost
template <typename To, typename From>
std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From> &&
                 std::is_unsigned_v<To> && std::is_signed_v<From>, std::optional<To>>
try_narrow_roundtrip_sfinae(const From& from) noexcept
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) return std::nullopt;
    if (from < 0) return std::nullopt;      // signed -> unsigned: sign lost
    return to;
}

// 4) To signed, From unsigned -> overflow wrap
template <typename To, typename From>
std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From> &&
                 std::is_signed_v<To> && std::is_unsigned_v<From>, std::optional<To>>
try_narrow_roundtrip_sfinae(const From& from) noexcept
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) return std::nullopt;
    if (to < 0) return std::nullopt;        // unsigned -> signed: overflow wrap
    return to;
}

// 5) non-integral (float<->float, float<->int, etc.)
template <typename To, typename From>
std::enable_if_t<!std::is_integral_v<To> || !std::is_integral_v<From>, std::optional<To>>
try_narrow_roundtrip_sfinae(const From& from) noexcept
{
    const auto to = static_cast<To>(from);
    if (static_cast<From>(to) != from) return std::nullopt;
    return to;
}

// print a value to a stream (promote char/bool to int for readability)
template <typename T>
void print_val(std::ostream& os, const T& v) {
    if constexpr (std::is_same_v<T, char> || std::is_same_v<T, unsigned char> ||
                  std::is_same_v<T, signed char> || std::is_same_v<T, bool>)
        os << static_cast<int>(v);
    else
        os << v;
}

// RUN one case through a given implementation IMPL (a function template name).
// Just prints the result; no expected value is required.
#define RUN(IMPL, desc, To, From, val)                                      \
    do {                                                                    \
        auto r = IMPL<To, From>(val);                                       \
        std::cout << "  " << (desc) << " -> "                               \
                  << (r.has_value() ? "kept" : "lost");                     \
        if (r.has_value()) { std::cout << "  value="; print_val(std::cout, *r); } \
        std::cout << "\n";                                                  \
    } while (0)

// Cases that work for ANY implementation (integral + floating point).
#define UNIVERSAL_CASES(IMPL)                                               \
    RUN(IMPL, "int 100         -> short       ", short, int, 100);           \
    RUN(IMPL, "int 40000       -> short (of)  ", short, int, 40000);        \
    RUN(IMPL, "int 16777217    -> float (pl)  ", float, int, 16777217);     \
    RUN(IMPL, "int 100         -> float       ", float, int, 100);          \
    RUN(IMPL, "double 1.5      -> int (ft)    ", int, double, 1.5);        \
    RUN(IMPL, "double 2.0      -> int         ", int, double, 2.0);         \
    RUN(IMPL, "float 1.1       -> double      ", double, float, 1.1f);      \
    RUN(IMPL, "int 300         -> schar (of)  ", signed char, int, 300);    \
    RUN(IMPL, "int 2           -> bool (n01)  ", bool, int, 2);            \
    RUN(IMPL, "bool true       -> int         ", int, bool, true);          \
    RUN(IMPL, "int -1          -> unsigned    ", unsigned, int, -1);        \
    RUN(IMPL, "unsigned 4e9    -> int (of)    ", int, unsigned, 4000000000u); \
    RUN(IMPL, "int -1          -> ushort      ", unsigned short, int, -1);  \
    RUN(IMPL, "uchar 200       -> schar (sl)  ", signed char, unsigned char, (unsigned char)200);

// Cases for std::in_range based impl (standard integer types only; char/bool
// are rejected by its static_assert, so they are excluded here).
#define INT_CASES(IMPL)                                                     \
    RUN(IMPL, "int 100         -> short       ", short, int, 100);          \
    RUN(IMPL, "int 40000       -> short (of)  ", short, int, 40000);        \
    RUN(IMPL, "int -1          -> unsigned    ", unsigned, int, -1);        \
    RUN(IMPL, "unsigned 4e9    -> int (of)    ", int, unsigned, 4000000000u); \
    RUN(IMPL, "int -1          -> ushort      ", unsigned short, int, -1);

// Extra boundary / precision / sign cases.
#define EXTRA_CASES(IMPL)                                                  \
    RUN(IMPL, "unsigned 2147483647-> int        ", int, unsigned, 2147483647u); \
    RUN(IMPL, "unsigned 2147483648-> int (of)   ", int, unsigned, 2147483648u); \
    RUN(IMPL, "long long 1e18  -> int (of)      ", int, long long, 1000000000000000000LL); \
    RUN(IMPL, "int -128        -> schar         ", signed char, int, -128); \
    RUN(IMPL, "int -129        -> schar (of)    ", signed char, int, -129); \
    RUN(IMPL, "int 127         -> schar         ", signed char, int, 127);  \
    RUN(IMPL, "int 128         -> schar (of)    ", signed char, int, 128);  \
    RUN(IMPL, "uchar 255       -> schar (sl)    ", signed char, unsigned char, (unsigned char)255); \
    RUN(IMPL, "schar -1        -> int           ", int, signed char, (signed char)-1); \
    RUN(IMPL, "uchar 255       -> int           ", int, unsigned char, (unsigned char)255); \
    RUN(IMPL, "ushort 65535    -> short (of)    ", short, unsigned short, (unsigned short)65535); \
    RUN(IMPL, "double 0.1      -> float (pl)    ", float, double, 0.1);    \
    RUN(IMPL, "double 0.5      -> int (ft)      ", int, double, 0.5);      \
    RUN(IMPL, "double 3.0      -> int           ", int, double, 3.0);      \
    RUN(IMPL, "bool false      -> int           ", int, bool, false);      \
    RUN(IMPL, "int 0           -> bool          ", bool, int, 0);          \
    RUN(IMPL, "int 1           -> bool          ", bool, int, 1);          \
    RUN(IMPL, "bool true       -> unsigned      ", unsigned, bool, true);  \
    RUN(IMPL, "unsigned 0      -> int           ", int, unsigned, 0u);

int main()
{
    std::cout << "=== try_narrow (original, flawed) ===\n";
    UNIVERSAL_CASES(try_narrow)

    std::cout << "\n=== try_narrow_inrange (std::in_range) ===\n";
    std::cout << "(standard integer types only)\n";
    INT_CASES(try_narrow_inrange)

    std::cout << "\n=== try_narrow_roundtrip (minimal fix) ===\n";
    UNIVERSAL_CASES(try_narrow_roundtrip)

    std::cout << "\n--- extra cases: try_narrow (original, flawed) ---\n";
    EXTRA_CASES(try_narrow)
    std::cout << "\n--- extra cases: try_narrow_roundtrip (minimal fix) ---\n";
    EXTRA_CASES(try_narrow_roundtrip)

    std::cout << "\n=== try_narrow_roundtrip_sfinae (SFINAE overloads) ===\n";
    UNIVERSAL_CASES(try_narrow_roundtrip_sfinae)
    std::cout << "\n--- extra cases: try_narrow_roundtrip_sfinae ---\n";
    EXTRA_CASES(try_narrow_roundtrip_sfinae)

    return 0;
}
