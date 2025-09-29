#include <iostream>
#include <variant>

using namespace std;

struct InvalidMode {
    bool operator==(const InvalidMode& other) const {
        return true;
    }
    bool operator!=(const InvalidMode& other) const { return !(*this == other); }
};

struct ConstMode {
    bool operator==(const ConstMode& other) const {
        return true;
    }
    bool operator!=(const ConstMode& other) const { return !(*this == other); }
};

using Mode = variant<InvalidMode, ConstMode>;

int main() {
    Mode mode1 = InvalidMode{};
    Mode mode2 = ConstMode{};

    cout << "Mode{InvalidMode{}} == Mode{ConstMode{}}: "
              << (mode1 == mode2 ? "true" : "false") << endl;

    cout << "Mode{InvalidMode{}} != Mode{ConstMode{}}: "
              << (mode1 != mode2 ? "true" : "false") << endl;

    // Additional checks for clarity
    cout << "\nAdditional checks:" << endl;
    cout << "mode1 holds InvalidMode: " << (holds_alternative<InvalidMode>(mode1) ? "true" : "false") << endl;
    cout << "mode2 holds ConstMode: " << (holds_alternative<ConstMode>(mode2) ? "true" : "false") << endl;
    cout << "mode1 index: " << mode1.index() << endl;
    cout << "mode2 index: " << mode2.index() << endl;

    return 0;
}
