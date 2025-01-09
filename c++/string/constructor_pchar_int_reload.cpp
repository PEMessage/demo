#include <cstdint>
#include <iostream>
using namespace std;
int main (int argc, char *argv[]) {
    const char str[] = "123";
    // https://cplusplus.com/reference/string/string/string/
    // (5) from buffer
    // Copies the first n characters from the array of characters pointed by s.
    std::string a = {str, 1}  ;
    cout << a << endl; 
    std::string b = {str, 2}  ;
    cout << b << endl; 
    std::string c = {str, 3}  ;
    cout << c << endl; 
    std::string d = {str, 4}  ;
    cout << d << endl; 
    std::string f = {str, 10}  ;
    cout << f << endl; 
    std::string g (str, 10)  ;
    cout << g << endl; 
    cout << "str pointer is: " << reinterpret_cast<uintptr_t>(str) << endl;
    cout << "std::string::data is: " << reinterpret_cast<uintptr_t>(g.data()) <<  endl;

    // 1
    // 12
    // 123
    // 123
    // 123g
    // 123g
    // str pointer is: 140725560342580
    // std::string::data is: 140725560342560

    return 0;
}



