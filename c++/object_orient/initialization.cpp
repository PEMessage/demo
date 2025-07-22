#include <string.h>
#include <utility>
#include <iostream>


void demo_cworld() {
    int a = 0; // for scalar, we have `value initization`
    (void)a;
    int b[3] = {1, 2, 3}; // for aggregate, we have `aggregate initization`
                          // struct
                          // array
    (void)b;
}


using namespace std;


// If a class has:
//  1. no-user declare constructor
//  2. no private or protect none-static data member
//  3. baseclass
//  4. virtual func

class DemoStr {
public:
    #define DEFAULT_TYPE 1
    // Test it by `for x in {1..100} ; do ./initialization.out; done |& rg '^\|\d:' | sort | uniq`
    #if (DEFAULT_TYPE == 1)
    // 1 -- default: scalar will be vacuous for 
    // in default initialization
    //    DemoStr a; --> un-init
    //    static DemoStr a; --> init
    //    DemoStr a = DemoStr(); --> init
    //    DemoStr *d = new DemoStr(); --> init
    //    DemoStr e {} --> init
    DemoStr() = default; 
    #elif (DEFAULT_TYPE == 2)
    // 2 -- user defined, just called it, not init it
    //    DemoStr a; --> un-init
    //    static DemoStr a; --> do-init
    //    DemoStr a = DemoStr(); --> un-init
    //    DemoStr *d = new DemoStr(); --> un-init
    //    DemoStr e {} --> un-init
    DemoStr() {
        cout << __PRETTY_FUNCTION__ << endl;
    }
    #elif (DEFAULT_TYPE == 3)
    // 3 -- user defined, init all
    //    all will be init
    DemoStr() {
        cout << __PRETTY_FUNCTION__ << endl;
        length_ = 0;
        str_ = nullptr;
    }
    #endif

    // Rule of 3
    DemoStr(const DemoStr& other): DemoStr(other.str_, other.length_){
        cout << __PRETTY_FUNCTION__ << endl;
    }
    DemoStr& operator=(const DemoStr& other) {
        cout << __PRETTY_FUNCTION__ << endl;
        if (this != &other) {
            DemoStr temp(other);                    // copy construction (may throw)
            delete[] str_;

            length_ = other.length_;
            if (length_ > 0) {
                str_ = new char[length_ + 1];
                memcpy(str_, other.str_, length_);
                str_[length_] = '\0';
            }

        }
        return *this;
    }

    ~DemoStr() {
        cout << __PRETTY_FUNCTION__ << endl;
        delete[] str_;
    }

    // user defined constructor
    DemoStr(const char *str): DemoStr(str, strlen(str)) {
        cout << __PRETTY_FUNCTION__ << endl;

    }
    DemoStr(const char *str, size_t length) {
        cout << __PRETTY_FUNCTION__ << endl;
        length_ = length;
        if (length_ > 0) {
            str_ = new char[length_ + 1];
            memcpy(str_, str, length_);
            str_[length_] = '\0';
        }
    }

    operator char *() {
        cout << __PRETTY_FUNCTION__ << endl;
        return str_;
    }
// private: // rule 2 // do not used private here to allow test case direct access
    size_t length_;
    char* str_;
};


void demo_DemoStr() {
    {
        cout << "\n========= DemoStr a(\"123\");" << endl;
        DemoStr a("123"); // direct initialization
        DemoStr b {"123"};
        cout << a << endl;
        cout << b << endl;
    }
    {
        cout << "\n========= DemoStr a = \"123\";" << endl;
        DemoStr a = "123"; // copy initialization
                           // not compile pass compile if `DemoStr::DemoStr(const char*)` set to explicit
        cout << a << endl;
    }
    {
        cout << "\n========= DemoStr a / static DemoStr a / DemoStr a = DemoStr(); / new DemoStr() / DemoStr a {}" << endl;
        // this will do default-initialization,
        // default-initialization it's member, vacuous initialization if meet scalar vacrous(none static)
        DemoStr a; 
        cout << "|1: " << a.length_ << endl;

        // Still default-initialization, but due to static, it has 
        static DemoStr b;
        cout << "|2: " << b.length_ << endl;

        // this will do value-initialization
        // value-initialization it's member, zero initialization if meet scalar
        DemoStr c = DemoStr(); 
        cout << "|3: " << c.length_ << endl;
        // cout << a.length_ << endl;
        DemoStr *d = new DemoStr();
        cout << "|4: " << d->length_ << endl;

        DemoStr e {}; // or DemoStr e = {}
        cout << "|5: " << e.length_ << endl;
        delete d;

    }
}


int main (int argc, char *argv[]) {
    demo_cworld();
    demo_DemoStr();
    
    return 0;
}
