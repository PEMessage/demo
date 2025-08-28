#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <type_traits>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <cstring>
#include <vector>
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
    bool test;
};
VISITABLE_STRUCT(Options, input, output, offset, test);


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

        // Split arg by = and @
        size_t equal_pos = arg.find('=');
        size_t at_pos = arg.find('@');
        bool is_option = false;

        string key {};
        string value {};

        if (equal_pos != string::npos) {
            key = arg.substr(0, equal_pos);
            value = arg.substr(equal_pos + 1);
        } else {
            key = arg;
        }

        if (at_pos == 0  ) {
            key = key.substr(at_pos + 1);
            is_option = true;
        }

        if (is_option) {
            visit_struct::for_each(op, [&](const char* name, auto& member) {
                string field_name(name);
                using field_type = std::decay_t<decltype(member)>;
                if (field_name == key) {
                    if constexpr (std::is_same_v<field_type, bool>) {
                        member = true;
                    } else {
                        member = string_to_value<field_type>(value);
                    }
                }
            });
        } else if (!key.empty() && !value.empty()) {
            kv[key] = value;
        } else {
           (void) 0; // pass
        }
    }
    return ret;
}

template<typename T>
void modify_field(T& struct_instance, const KVMaps& kv_pairs) {
    visit_struct::for_each(struct_instance, [&](const char* name, auto& member) {
        std::string field_name(name);
        auto it = kv_pairs.find(field_name);
        if (it != kv_pairs.end()) {
            using MemberType = std::decay_t<decltype(member)>;
            member = string_to_value<MemberType>(it->second);
        }
    });
}


vector<char> readfile(const string& file_path) {
    ifstream file {file_path, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for readling: " + file_path);
    }
    const auto file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(file_size);
    if (!file.read(buffer.data(), file_size)) {
        throw runtime_error("Failed to read file: " + file_path);
    }

    return buffer;
}

void writefile(const string& file_path, const vector<char>& content) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Failed to open file for writing: " + file_path);
    }

    if (!file.write(content.data(), content.size())) {
        throw runtime_error("Failed to write to file: " + file_path);
    }
}


VISITABLE_STRUCT(winsize, ws_row, ws_col, ws_xpixel, ws_ypixel);
using FILE_FORMAT = winsize;

int main(int argc, char* argv[]) {
    try {
        auto [op, kv] = parse_key_value_args(argc, argv);
        cout << "Option: " << op << endl;
        cout << "KVMaps: " << kv << endl;

        vector<char> content = readfile(op.input);
        if (op.offset + sizeof(FILE_FORMAT) > content.size()) { 
            throw runtime_error(string("Not enough bytes to process ") +
                    ". Offset: " + to_string(op.offset) +
                    ", Required: " + to_string(sizeof(FILE_FORMAT)) +
                    ", Available: " + to_string(content.size()));
        }

        FILE_FORMAT &object = *reinterpret_cast<FILE_FORMAT *>(content.data() + op.offset);
        cout << "Format: " << object << endl;

        if (!kv.empty()) {
            modify_field(object, kv);
            cout << "Format After Modify: " << object << endl;
        }
        if(!op.output.empty()) {
            writefile(op.output, content);
            cout << "Write to file: " << op.output << endl;
        }

    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        // throw;
        return 1;
    }

}
