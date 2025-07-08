

#include <iostream>
#include <sstream>
#include  <iomanip>



// void print_kcv(const std::string &kcv) {
//     std::string tmp;
//     for ( auto x : kcv ) {
//         tmp =
//     }

// }


static std::string bin2string(const std::string &s) {
    std::ostringstream ss;
    size_t n = 0 ;
    std::string::const_iterator i ;
    for (i = s.cbegin(), n = 0  ; i != s.cend() ;  i++, n++ ) {
        if (  n != 0  &&  n % 8 == 0 ) {
            ss << std::endl ;
        }
        ss << "0x" ;
        ss << std::setfill('0') << std::setw(2)  << std::hex << int(*i) ;

        if ( i+1 != s.cend() ) {
            ss << ", " ;
        }
    }
    std::string result = ss.str();
    return result;
}
