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
    //  if not found in rest, propagate -1; otherwise add 1
    static constexpr int propagated_index = (rest_index == -1) ? -1 : rest_index + 1;

public:
    static constexpr int value = is_match ? 0 : propagated_index;
};


template<typename T, typename... Args>
constexpr inline int find_first_class() {
    static_assert(find_first_index<T, Args...>::value != -1, "find_first_class: Fail to founed");
    return find_first_index<T, Args...>::value;
}



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

    // Class not in variant
    // static_assert(-1 == variant_index<Mode3, Mode>::value, "AssertFail");
    static_assert(0 == find_first_class<Mode0, Mode0, Mode1>(), "AssertFail");
    static_assert(1 == find_first_class<Mode1, Mode0, Mode1>(), "AssertFail");
    // find_first_class<Mode3, Mode0, Mode1>();

    return 0;
}
