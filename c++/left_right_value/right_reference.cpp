// q-gcc: -fno-elide-constructors -- 
#include <iostream>

using namespace std;

class X {
public:
    X() { std::cout << "X ctor" << std::endl; }
    X(const X& x) { std::cout << "X copy ctor" << std::endl; }
    ~X() { std::cout << "X dtor" << std::endl; }
    void show() { std::cout << "Show X" << std::endl; }
};

X make_x() {
    X x1;
    return x1;
}



int main() {
    {
        cout << "right reference!!" << endl;
        X &&x2 = make_x();
        x2.show();
    }
    {
        cout << "normal copy ctor!!" << endl;
        X x3 = make_x();
        x3.show();
    }
}
