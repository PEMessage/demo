#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cstring>
#include <vector>
#include <memory>
#include "visit_struct/visit_struct.hpp"


using namespace std;

template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& map) {
    os << "{";
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) os << ", ";
        os << it->first << ": " << it->second;
    }
    os << "}";
    return os;
}

template<typename T,
         typename = std::enable_if_t<visit_struct::traits::is_visitable<T>::value>>
std::ostream& operator<<(std::ostream& os, const T& t)
{
    int i = 0;
    os << "{";
    visit_struct::for_each(t, [&](const char* name, const auto& value) {
        if (i != 0) {os << ", ";}
        os << name << ":" << value;
        i++;
    });
    os << "}";
    return os;
}

using KVMaps = map<std::string, std::string>;
struct Options {
    string input;
    string output;
    int offset;
};
VISITABLE_STRUCT(Options, input, output, offset);


template<typename T>
T string_to_value(const std::string& str) {
    std::istringstream iss(str);
    T value;
    iss >> value;
    return value;
}


pair<Options, KVMaps> parse_key_value_args(int argc, char* argv[]) {
    pair<Options, KVMaps> ret {};
    Options &op = ret.first;
    KVMaps &kv = ret.second;
    for (int i = 1; i < argc; i++) {

        string arg = argv[i];
        if (arg.empty()) {continue;}

        size_t pos = arg.find('=');
        if (pos != string::npos) {
            string key = arg.substr(0, pos);
            string value = arg.substr(pos + 1);
            kv[key] = value;
        } else if (arg.find('@') == 0 ) {
            visit_struct::for_each(op, [&](const char* name, auto& member) {
                string field_name(name);
                using field_type = std::decay_t<decltype(member)>;

                if ( "@" + field_name == arg ) {
                    member = string_to_value<field_type>(arg);
                }
            });
        } else {
            (void)0; // empty pass
        }
    }
    return ret;
}

int main(int argc, char* argv[]) {
    auto [op, kv] = parse_key_value_args(argc, argv);
    cout << kv;
    cout << op;

}
