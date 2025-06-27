// q-gcc: -fno-elide-constructors -O0 -- 
#include <iostream>
#include <utility>

using namespace std;

class X {
public:
    X() { std::cout << "X default ctor (" << this << ")" << std::endl; }
    X(X& x)  { std::cout << "X copy ctor (" << this << ") from (" << &x << ")" << std::endl; }  // 1
    X(X&& x) { std::cout << "X move ctor (" << this << ") from (" << &x << ")" << std::endl; }  // 2
    X(const X& x)  { std::cout << "X const copy ctor (" << this << ") from (" << &x << ")" << std::endl; } // 3
    X(const X&& x) { std::cout << "X const move ctor (" << this << ") from (" << &x << ")" << std::endl; } // 4

    ~X() { std::cout << "X dtor (" << this << ")" << std::endl; }
    void show()             { std::cout << "Show X (" << this << ")" << std::endl; }
    void show_const() const { std::cout << "Show X (" << this << ")" << std::endl; }
};

X make_x() {
    X x1;
    return x1;
}

const X make_x_const() {
    const X x1;
    return x1;
}

X make_x_notmp() {
    return X();
}



int main() {
    {
        cout << endl;
        cout << "--- X &&x = make_x()" << endl;
        X &&x = make_x(); // if comment some constructors, it will fallback
                          // priority is : 2 -> 4 -> 1 -> 3
                          // move -> copy and none-const -> const
        x.show();
        cout << endl;
    }
    {
        cout << endl;
        cout << "--- X x = make_x()" << endl;
        X x = make_x(); // if comment some constructors, it will fallback
                          // priority is : 2 -> 4 -> 1 -> 3
                          // move -> copy and none-const -> const
        x.show();
    }
    {
        cout << endl;
        cout << "--- X x = make_x_const()" << endl;
        X x = make_x_const(); // return a const, but still using move+none-const
        x.show();
    }
    {
        cout << endl;
        cout << "--- const X &&x = make_x_const()" << endl;
        // X &&x = make_x_const(); // compiler error, binding reference of type ‘X&&’ to ‘const X’ discards qualifiers
        const X &&x = make_x_const();
        // x.show();  // compiler error
        x.show_const();
    }
    {
        cout << endl;
        cout << "--- X &&x = make_x_notmp() // optimize to X x;" << endl;
        X &&x = make_x_notmp();
        x.show();
        cout << endl;
    }
    {
        cout << endl;
        cout << "--- X x = make_x_notmp() // optimize to X x;" << endl;
        X x = make_x_notmp();
        x.show();
    }
    {
        cout << endl;
        cout << "--- X x2 = move(x)" << endl;
        X x;
        X x2 = move(x);
        x2.show();
    } // after move x2/x will both call deconstruct
}
