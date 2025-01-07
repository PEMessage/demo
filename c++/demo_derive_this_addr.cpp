
#include <iostream>
using namespace std;

struct  A {
    int a;
    A() {
        std::cout << "A created" << endl;
        std::cout << this << endl;
    }
    ~A() {
        std::cout << "A destoryed";
        std::cout << this << endl;
    }
};

struct  B {
    int b;
    B() {
        std::cout << "B created" << endl;
        std::cout << this << endl;
    }
    ~B() {
        std::cout << "B destoryed" << endl;
        std::cout << this << endl;
    }
};

struct C: virtual A, virtual B {
};



int main (int argc, char *argv[]) {
    std::cout << "----------" << endl;
    C c;
    std::cout << "----------" << endl;
    std::cout << endl;
    
    return 0;
}
