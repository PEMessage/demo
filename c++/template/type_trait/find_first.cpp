#include <memory>
#include <type_traits>
#include <variant>

// ================================================
// [].find() but using recusive template
// ================================================
template<typename T, typename... Args>
struct find_first_index;

// Base case - type not found in pack
template<typename T>
struct find_first_index<T> {
    static constexpr int value = -1;
};


template<typename T, typename First, typename... Rest>
struct find_first_index<T, First, Rest...> {
private:
    // Break down the logic for better readability
    static constexpr bool is_match = std::is_same_v<T, First>;
    static constexpr int rest_index = find_first_index<T, Rest...>::value;

public:
    static constexpr int value = is_match ? 0 : (rest_index + 1);
};



// ================================================
// variant_index
// ================================================
template<typename T, typename Variant>
struct variant_index;

template<typename T, typename... Types>
struct variant_index<T, std::variant<Types...>> {
    static constexpr int value = find_first_index<T, Types...>::value;
};

struct Mode0 {};
struct Mode1 {};
struct Mode2 {};
struct Mode3 {};

using Mode = std::variant<Mode0, Mode1, Mode2> ;

int main (int argc, char *argv[]) {
    static_assert(0 == variant_index<Mode0, Mode>::value, "AssertFail");
    static_assert(1 == variant_index<Mode1, Mode>::value, "AssertFail");
    static_assert(2 == variant_index<Mode2, Mode>::value, "AssertFail");

    // char (*__kaboom)[variant_index<Mode3, Mode>::value] = 1;
    // Undefine behavior if not exist in varint
    static_assert(2 == variant_index<Mode3, Mode>::value, "AssertFail");


    return 0;
}
