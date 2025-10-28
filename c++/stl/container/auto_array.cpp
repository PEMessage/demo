#include <vector>
#include <string>
#include <array>
#include <string>
#include <iostream>


template<typename T, typename... Args>
constexpr auto make_array(Args&&... args) -> std::array<T, sizeof...(Args)> {
    return {std::forward<Args>(args)...};
}

const auto PATHS = make_array<std::string>(
    "/proc/touchpanel_info",
    "/proc/touchpanel_info1"
);

using namespace std;

int main (int argc, char *argv[]) {
    static_assert(std::size(PATHS) == 2);
    cout << std::size(PATHS) << endl;

    return 0;
}
