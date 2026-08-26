// q-gcc: -std=c++20 --
#include <optional>
#include <iostream>
#include <type_traits>
#include <limits>
#include <utility>

// ===========================================================================
// Implementations
// ===========================================================================

// Original, flawed: the round-trip check misses signed/unsigned wrap-around.
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

// Out-parameter version: returns bool (true == success), writes into the first
// reference argument. Only assigns on success.
template <typename To, typename From>
bool try_narrow_into(To& to, const From& from) noexcept
{
    const auto tmp = static_cast<To>(from);   // work on a temp first
    if (static_cast<From>(tmp) != from) {
        return false;
    }
    // minimal fix: detect signed/unsigned wrap-around that round-trips back
    if constexpr (std::is_unsigned_v<To> && std::is_signed_v<From>) {
        if (from < 0) {
            return false;               // signed -> unsigned: sign lost
        }
    }
    if constexpr (std::is_signed_v<To> && std::is_unsigned_v<From>) {
        if (tmp < 0) {
            return false;               // unsigned -> signed: overflow wrap
        }
    }
    to = tmp;   // only assign the output reference on success
    return true;
}

// ===========================================================================
// Test harness (X-macro)
// ===========================================================================

// print a value to a stream (promote char/bool to int for readability)
template <typename T>
void print_val(std::ostream& os, const T& v) {
    if constexpr (std::is_same_v<T, char> || std::is_same_v<T, unsigned char> ||
                  std::is_same_v<T, signed char> || std::is_same_v<T, bool>)
        os << static_cast<int>(v);
    else
        os << v;
}

// Handler for optional-returning implementations. The implementation name is
// supplied via the IMPL_FN macro (defined right before ALL_CASES/INT_CASES).
#define OPT_CASE(desc, To, From, val)                                        \
    do {                                                                     \
        auto r = IMPL_FN<To, From>(val);                                     \
        std::cout << "  " << (desc) << " -> "                                \
                  << (r.has_value() ? "kept" : "lost");                     \
        if (r.has_value()) { std::cout << "  value="; print_val(std::cout, *r); } \
        std::cout << "\n";                                                   \
    } while (0);

// Handler for the out-param form (try_narrow_into).
#define INTO_CASE(desc, To, From, val)                                       \
    do {                                                                     \
        To _o{}; bool _ok = try_narrow_into(_o, val);                        \
        std::cout << "  " << (desc) << " -> "                               \
                  << (_ok ? "kept" : "lost");                               \
        if (_ok) { std::cout << "  value="; print_val(std::cout, _o); }     \
        std::cout << "\n";                                                   \
    } while (0);

// The single source of truth for all test cases. X is a per-case handler macro
// taking (desc, To, From, val). OPT_CASE and INTO_CASE are just two different X
// handlers, so they share this one list (RUN and RUN2 merged via X-macro).
#define ALL_CASES(X)                                                         \
    X("int 100         -> short       ", short, int, 100)                    \
    X("int 40000       -> short (of)  ", short, int, 40000)                 \
    X("int 16777217    -> float (pl)  ", float, int, 16777217)              \
    X("int 100         -> float       ", float, int, 100)                   \
    X("double 1.5      -> int (ft)    ", int, double, 1.5)                 \
    X("double 2.0      -> int         ", int, double, 2.0)                  \
    X("float 1.1       -> double      ", double, float, 1.1f)               \
    X("int 300         -> schar (of)  ", signed char, int, 300)             \
    X("int 2           -> bool (n01)  ", bool, int, 2)                      \
    X("bool true       -> int         ", int, bool, true)                   \
    X("int -1          -> unsigned    ", unsigned, int, -1)                 \
    X("unsigned 4e9    -> int (of)    ", int, unsigned, 4000000000u)        \
    X("int -1          -> ushort      ", unsigned short, int, -1)           \
    X("uchar 200       -> schar (sl)  ", signed char, unsigned char, (unsigned char)200) \
    X("unsigned 2147483647-> int      ", int, unsigned, 2147483647u)        \
    X("unsigned 2147483648-> int (of) ", int, unsigned, 2147483648u)        \
    X("long long 1e18  -> int (of)    ", int, long long, 1000000000000000000LL) \
    X("int -128        -> schar       ", signed char, int, -128)             \
    X("int -129        -> schar (of)  ", signed char, int, -129)             \
    X("int 127         -> schar       ", signed char, int, 127)              \
    X("int 128         -> schar (of)  ", signed char, int, 128)              \
    X("uchar 255       -> schar (sl)  ", signed char, unsigned char, (unsigned char)255) \
    X("schar -1        -> int         ", int, signed char, (signed char)-1)  \
    X("uchar 255       -> int         ", int, unsigned char, (unsigned char)255) \
    X("ushort 65535    -> short (of)  ", short, unsigned short, (unsigned short)65535) \
    X("double 0.1      -> float (pl)  ", float, double, 0.1)                \
    X("double 0.5      -> int (ft)    ", int, double, 0.5)                  \
    X("double 3.0      -> int         ", int, double, 3.0)                  \
    X("bool false      -> int         ", int, bool, false)                  \
    X("int 0           -> bool        ", bool, int, 0)                      \
    X("int 1           -> bool        ", bool, int, 1)                      \
    X("bool true       -> unsigned    ", unsigned, bool, true)              \
    X("unsigned 0      -> int         ", int, unsigned, 0u)

// standard-integer-only cases (std::in_range rejects char/bool)
#define INT_CASES(X)                                                         \
    X("int 100         -> short       ", short, int, 100)                    \
    X("int 40000       -> short (of)  ", short, int, 40000)                  \
    X("int -1          -> unsigned    ", unsigned, int, -1)                 \
    X("unsigned 4e9    -> int (of)    ", int, unsigned, 4000000000u)        \
    X("int -1          -> ushort      ", unsigned short, int, -1)

int main()
{
    std::cout << "=== try_narrow (original, flawed) ===\n";
    #define IMPL_FN try_narrow
    ALL_CASES(OPT_CASE)
    #undef IMPL_FN

    std::cout << "\n=== try_narrow_inrange (std::in_range) ===\n";
    std::cout << "(standard integer types only)\n";
    #define IMPL_FN try_narrow_inrange
    INT_CASES(OPT_CASE)
    #undef IMPL_FN

    std::cout << "\n=== try_narrow_roundtrip (minimal fix) ===\n";
    #define IMPL_FN try_narrow_roundtrip
    ALL_CASES(OPT_CASE)
    #undef IMPL_FN

    std::cout << "\n=== try_narrow_roundtrip_sfinae (SFINAE overloads) ===\n";
    #define IMPL_FN try_narrow_roundtrip_sfinae
    ALL_CASES(OPT_CASE)
    #undef IMPL_FN

    std::cout << "\n=== try_narrow_into (out-param, returns bool) ===\n";
    ALL_CASES(INTO_CASE)

    return 0;
}
