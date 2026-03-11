#include <iostream>
#include <variant>
#include <stdexcept>
#include <utility>
#include <assert.h>

struct Mode0 {};
struct Mode1 {};
struct Mode2 {};
struct Mode3 {};
struct Mode4 {};
using Mode = std::variant<Mode0, Mode1, Mode2>;


/**
 * @brief Creates a variant instance by selecting the alternative at the specified index
 *
 * @tparam Variant The variant type containing all possible alternative types
 * @param n Zero-based index indicating which variant alternative to construct
 * @return Variant A variant containing a default-constructed instance of the type at index n
 *
 * @throws Assertion failure if n is out of bounds (0 <= n < std::variant_size_v<Variant>)
 *
 * @example
 * using Mode = std::variant<Mode0, Mode1, Mode2, Mode3, Mode4>;
 * auto mode = variantByIndex<Mode>(2); // Returns variant containing Mode2{}
 *
 * @note Expands at compile time to a series of conditional checks using fold expression:
 * (n == 0 ? (result = Mode0{}, true) : false) ||
 * (n == 1 ? (result = Mode1{}, true) : false) ||
 * (n == 2 ? (result = Mode2{}, true) : false) ||
 * (n == 3 ? (result = Mode3{}, true) : false) ||
 * (n == 4 ? (result = Mode4{}, true) : false)
 */
template<typename Variant>
Variant variantByIndex(size_t n) {
    assert(n >= 0 && n <= std::variant_size_v<Variant>);

    return [n]<std::size_t... Is>(std::index_sequence<Is...>) {
        Variant result;
        ((n == Is ? (result = std::variant_alternative_t<Is, Variant>{}, true) : false) || ...);
        return result;
    }(std::make_index_sequence<std::variant_size_v<Variant>>{});
}


int main (int argc, char *argv[]) {

    Mode mode1 = variantByIndex<Mode>(1);
    Mode mode2 = variantByIndex<Mode>(2);

    std::cout << mode1.index() << std::endl;
    std::cout << mode2.index() << std::endl;
    return 0;
}
