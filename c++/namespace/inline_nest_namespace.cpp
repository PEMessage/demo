
#include <iostream>

using namespace std;
    

namespace lib {
    namespace V1 {
        void foo() {
        cout << "This is: " << __PRETTY_FUNCTION__  << endl;
        }
    }

    // See: https://www.ibm.com/docs/en/zos/3.1.0?topic=only-inline-namespace-definitions-c11
    // Since C++11
    inline namespace V2 {
        void foo() {
            cout << "This is: " << __PRETTY_FUNCTION__  << endl;
        }
    }
}


// Since C++17
#if defined(__cplusplus) && __cplusplus >= 201703L
namespace ohos::middleware::lib {
    void foo() {
        cout << "This is: " << __PRETTY_FUNCTION__  << endl;
    }
}
#endif



int main (int argc, char *argv[]) {
    lib::V1::foo();
    lib::foo();
    #if defined(__cplusplus) && __cplusplus >= 201703L
    {
        ohos::middleware::lib::foo();
    }
    #endif
    return 0;
}
