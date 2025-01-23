
#include <iostream>
using namespace std;

class  A {
    public:
        void pub_func(const A &x) {
            cout << "public pub_func of: " << this  << endl;
            x.priv_func();
        }
    private:
        void priv_func() const {
            cout << "private priv_func of: " << this << endl;
        }
};

// prvivate function: two object of same class could call each other
// suitable for operation reload(more than one object)


int main (int argc, char *argv[]) {

    A a1;
    A a2;

    a1.pub_func(a2);
    a2.pub_func(a1);
    
    return 0;
}
