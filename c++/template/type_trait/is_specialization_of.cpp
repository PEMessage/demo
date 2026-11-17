#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ================================================
// is_specialization_of: does T name a specialization of the class template HKT?
//
//   is_specialization_of<std::vector, std::vector<int>>   // true
//   is_specialization_of<std::vector, std::optional<int>> // false
//
// Two styles:
//   1. variable template + partial specialization      -> yields a bool directly
//   2. struct trait + partial specialization           -> yields std::true_type/false_type
//
// Language level:
//   - variable templates:                C++14
//   - partial spec of variable template: C++14 (works in GCC 5 / Clang 3.4)
//   - template<...> typename HKT:        C++17 (C++14 can only spell `class HKT`)
//   - constexpr inline variable:         C++17 (drop `inline` for C++14)
//   - std::optional:                     C++17
//   - std::remove_cvref_t:               C++20 (we hand-roll one so C++17 works)
//
// So style 1 as written is C++17. The C++14 form of the same idea:
//   template <template <class...> class HKT, class T>
//   constexpr bool is_specialization_of = false;
//   template <template <class...> class HKT, class... Ts>
//   constexpr bool is_specialization_of<HKT, HKT<Ts...>> = true;
// ================================================

// std::remove_cvref_t is C++20; hand-roll it to stay C++17-compatible
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

static void print_bool(const char* label, bool value) {
    std::cout << "  " << label << " = " << std::boolalpha << value << '\n';
}


// ================================================
// Style 1: variable template (shortest, usable as a bool)
// ================================================
namespace style1 {

template <template <typename...> typename HKT, typename T>
constexpr inline bool is_specialization_of = false;

template <template <typename...> typename HKT, typename... Ts>
constexpr inline bool is_specialization_of<HKT, HKT<Ts...>> = true;

// Typical usage: strip cv/ref first so `const T&` / `T&&` also match
template <typename T>
constexpr inline bool is_optional_v =
    is_specialization_of<std::optional, remove_cvref_t<T>>;

template <typename T>
constexpr inline bool is_vector_v =
    is_specialization_of<std::vector, remove_cvref_t<T>>;

} // namespace style1


// ================================================
// Style 2: struct trait (has ::value, usable for tag dispatch / overloads)
// ================================================
namespace style2 {

template <template <class...> class HKT, class T>
struct is_specialization_of : std::false_type {};

template <template <class...> class HKT, class... Ts>
struct is_specialization_of<HKT, HKT<Ts...>> : std::true_type {};

template <template <class...> class HKT, class T>
constexpr bool is_specialization_of_v = is_specialization_of<HKT, T>::value;

template <typename T>
constexpr bool is_optional_v = is_specialization_of_v<std::optional, remove_cvref_t<T>>;

} // namespace style2


// ================================================
// Tests
// ================================================
void test_variable_template_style() {
    std::cout << "[style1: variable template]\n";
    print_bool("std::optional<int>", style1::is_optional_v<std::optional<int>>);
    print_bool("std::optional<std::string>", style1::is_optional_v<std::optional<std::string>>);
    print_bool("int", style1::is_optional_v<int>);

    print_bool("std::vector<int>", style1::is_vector_v<std::vector<int>>);
    print_bool("std::vector<std::optional<int>>", style1::is_vector_v<std::vector<std::optional<int>>>);
    print_bool("std::optional<int>", style1::is_vector_v<std::optional<int>>);

    // The bool can be used directly as a template argument: no ::value needed
    static_assert(style1::is_vector_v<std::vector<char>>);
    static_assert(!style1::is_vector_v<int>);
}

void test_struct_trait_style() {
    std::cout << "[style2: struct trait]\n";
    print_bool("std::optional<int>", style2::is_optional_v<std::optional<int>>);
    print_bool("int", style2::is_optional_v<int>);

    print_bool("std::vector<int>", style2::is_specialization_of_v<std::vector, std::vector<int>>);
    print_bool("std::pair<int, char>", style2::is_specialization_of_v<std::pair, std::pair<int, char>>);
    print_bool("std::map<int, int>", style2::is_specialization_of_v<std::map, std::map<int, int>>);

    // The trait itself is a type: inherit from it, use it for overloads, etc.
    static_assert(style2::is_specialization_of<std::vector, std::vector<int>>::value);
    static_assert(!style2::is_specialization_of<std::vector, std::optional<int>>::value);
    static_assert(std::is_base_of_v<std::true_type,
                                    style2::is_specialization_of<std::vector, std::vector<int>>>);
}

void test_cvref() {
    std::cout << "[cv / ref stripping]\n";
    // Without stripping, `const std::optional<int>&` cannot match HKT<Ts...>
    print_bool("is_optional_v<const std::optional<int>&> (raw)",
               style1::is_specialization_of<std::optional, const std::optional<int>&>);
    print_bool("is_optional_v<const std::optional<int>&> (stripped)",
               style1::is_optional_v<const std::optional<int>&>);
    print_bool("is_optional_v<std::optional<int>&&> (stripped)",
               style1::is_optional_v<std::optional<int>&&>);
}

void test_notes() {
    std::cout << "[notes: argument shapes]\n";
    // std::string is just basic_string<char, ...>
    print_bool("std::string is basic_string",
               style1::is_specialization_of<std::basic_string, std::string>);
    // but it is not a vector
    print_bool("std::string is vector", style1::is_vector_v<std::string>);

    // Alias templates are never deducible, so they cannot be matched here:
    //   style1::is_specialization_of<std::remove_cv_t, int>   // always false, useless

    // HKT is spelled `typename...`, so templates with non-type parameters fail:
    //   style1::is_specialization_of<std::array, std::array<int, 3>>  // compile error
}


// ================================================
// Compile-time macros: what the compiler actually provides
// ================================================
constexpr const char* standard_name() {
#if __cplusplus >= 202100L
    return "C++23 or later";
#elif __cplusplus >= 202002L
    return "C++20";
#elif __cplusplus >= 201703L
    return "C++17";
#elif __cplusplus >= 201402L
    return "C++14";
#else
    return "C++11 or earlier";
#endif
}

void test_compile_macros() {
    std::cout << "[compile macros]\n";
    std::cout << "  __cplusplus              = " << __cplusplus
              << "  -> " << standard_name() << '\n';
#ifdef __cpp_variable_templates
    std::cout << "  __cpp_variable_templates = " << __cpp_variable_templates << '\n';
#endif
#ifdef __cpp_inline_variables
    std::cout << "  __cpp_inline_variables   = " << __cpp_inline_variables << '\n';
#endif
#ifdef __cpp_if_constexpr
    std::cout << "  __cpp_if_constexpr       = " << __cpp_if_constexpr << '\n';
#endif
#ifdef __cpp_concepts
    std::cout << "  __cpp_concepts           = " << __cpp_concepts << '\n';
#endif
    std::cout << "  __GNUC__                 = " << __GNUC__ << '\n'
              << "  __GNUC_MINOR__           = " << __GNUC_MINOR__ << '\n'
              << "  __GNUC_PATCHLEVEL__      = " << __GNUC_PATCHLEVEL__ << '\n';
}

int main() {
    test_compile_macros();
    test_variable_template_style();
    test_struct_trait_style();
    test_cvref();
    test_notes();
    return 0;
}
