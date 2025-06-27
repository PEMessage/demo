// q-gcc: -fno-elide-constructors -- 
#include <iostream>
#include <utility>

using namespace std;

class X {
public:
    X() { std::cout << "X default ctor (" << this << ")" << std::endl; }
    X(const X& x)  { std::cout << "X const copy ctor (" << this << ") from (" << &x << ")" << std::endl; }
    X(const X&& x) { std::cout << "X const move ctor (" << this << ") from (" << &x << ")" << std::endl; }
    X(X& x)  { std::cout << "X copy ctor (" << this << ") from (" << &x << ")" << std::endl; }
    X(X&& x) { std::cout << "X move ctor (" << this << ") from (" << &x << ")" << std::endl; }

    ~X() { std::cout << "X dtor (" << this << ")" << std::endl; }
    void show() { std::cout << "Show X (" << this << ")" << std::endl; }
};

X make_x() {
    X x1;
    return x1;
}

X make_x_notmp() {
    return X();
}



int main() {
    {
        cout << endl;
        cout << "--- X &&x = make_x()" << endl;
        X &&x = make_x();
        x.show();
        cout << endl;
    }
    {
        cout << endl;
        cout << "--- X x = make_x()" << endl;
        X x = make_x();
        x.show();
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
    } // after move x2/x will de deconstruct
}
