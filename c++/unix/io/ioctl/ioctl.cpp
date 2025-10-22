#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <typeinfo>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include <string_view>
#include <map>
#include <vector>
#include <variant>
#include <functional>
#include <inttypes.h>

#include <asm-generic/int-ll64.h>

#include "visit_struct/visit_struct.hpp"

// ---------------------------------------
// 1.0 Helper Function
// ---------------------------------------
template <typename T>
constexpr auto type_name() {
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}



void print_buffer(void *ptr, size_t len) {
    unsigned char *addr = (unsigned char *)ptr;

    // Print the pointer address in hex (byte by byte)
    printf("Content: %p\n", ptr);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", ((unsigned char*)ptr)[i]);
        if (i % 8 == 8 -1) {
            printf("\n");
        }
    }
    printf("\n");
}

template<typename T>
T string_to_value(const std::string& str) {
    std::istringstream iss(str);
    T value;
    iss >> value;
    return value;
}

// Specialization for char (to handle numeric conversion)
template<>
char string_to_value<char>(const std::string& str) {
    return static_cast<char>(std::stoi(str));
}

// Specialization for unsigned char
template<>
unsigned char string_to_value<unsigned char>(const std::string& str) {
    return static_cast<unsigned char>(std::stoul(str));
}

// ---------------------------------------
// 1.1 Helper TypeTrait
// ---------------------------------------
template<typename T>
struct is_char_array : std::false_type {};

template<std::size_t N>
struct is_char_array<char[N]> : std::true_type {};

template<std::size_t N>
struct is_char_array<const char[N]> : std::true_type {};

template <class, class = void>
struct is_std_variant : std::false_type {};

template <class T>
struct is_std_variant<T, std::void_t<decltype(std::variant_size<T>::value)>>
    : std::true_type {};

template <typename T, template <typename...> class Template>
struct is_specialization : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

// Helper variable template
template <typename T, template <typename...> class Template>
inline constexpr bool is_specialization_v = is_specialization<T, Template>::value;

// ---------------------------------------
// 2. debug_print
// ---------------------------------------
template <class T>
void meta_print(const char* name, const T& value) {
    if constexpr(std::is_same_v<T, char> || std::is_same_v<T, unsigned char>) {
        std::cout << name << ": " << (int)value;
    } else {
        std::cout << name << ": " << value;
    }
}

// See: https://github.com/cbeck88/visit_struct/issues/26
template <class Value>
void  debug_print(const char* name, const Value& value, int indent = 0) {
    const std::string INDENT = std::string(indent * 2, ' ');
    if constexpr (visit_struct::traits::is_visitable<Value>::value) {
        std::cout << INDENT
            << name
            << ": "
            << "{"
            << std::endl;
        visit_struct::for_each(value, [&](const char* name, const auto& member) {
            debug_print(name, member, indent + 1);
        });
        std::cout << INDENT << "}"
            // << "  // " <<  type_name<Value>()
            << std::endl;
    } else if constexpr (std::is_array_v<Value> && !std::is_same_v<std::remove_extent_t<Value>, char>) {
            int i = 0;
            std::cout << INDENT
                << name
                << ": "
                << "[" << std::endl;
            for (const auto& element : value) {
                debug_print(std::to_string(i).c_str(), element, indent + 1);
                i++;
            }
            std::cout << INDENT << "]"
                // << "  // " <<  type_name<Value>()
                << std::endl;;
    } else if constexpr (is_specialization<Value, std::map>::value ) {
        std::cout << INDENT << name << ": {" << std::endl;
        for (const auto& [key, val] : value) {
            debug_print(key.c_str(), val, indent + 2);
        }
        std::cout << INDENT << "}" << std::endl;
    } else if constexpr(is_std_variant<Value>::value) {
        std::visit([&](const auto& v) {
            debug_print(name, v);
        }, value);
    } else {
        std::cout << INDENT;
        meta_print(name, value);
        std::cout << std::endl;
    }
}

// ---------------------------------------
// 3. modify_field
// ---------------------------------------

template<typename T>
void modify_struct_fields(T& struct_instance, const std::map<std::string, std::string>& kv_pairs) {
    visit_struct::for_each(struct_instance, [&](const char* name, auto& member) {
        std::string field_name(name);
        auto it = kv_pairs.find(field_name);
        if (it != kv_pairs.end()) {
            using MemberType = std::decay_t<decltype(member)>;
            try {
                member = string_to_value<MemberType>(it->second);
                std::cout << "Set " << field_name << " = " << it->second << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error converting value for " << field_name
                          << ": " << e.what() << std::endl;
            }
        }
    });
}


// ---------------------------------------
// 4. IOCTL Define
// ---------------------------------------
template<typename T>
struct ioctl_trait {
    static_assert(sizeof(T) == 0, "ioctl_trait not specialized for this type");
};


// 4.1 TIOCGWINSZ
// ---------------------------------------
VISITABLE_STRUCT(winsize, ws_row, ws_col, ws_xpixel, ws_ypixel);
#define IOCTL_CMD     TIOCGWINSZ
typedef struct winsize IOCTL_STRUCT;
#define IOCTL_FILE "/dev/tty"

template<>
struct ioctl_trait<winsize> {
    static constexpr unsigned long cmd = TIOCGWINSZ;
    static constexpr std::string_view path = "/dev/tty";
};


// 4.2 BINDER_FEATURE_SET
// ---------------------------------------
struct binder_feature_set {
    __u64 feature_set;
};
#define BINDER_FEATURE_SET _IOWR('b', 30, struct binder_feature_set)
VISITABLE_STRUCT(binder_feature_set, feature_set);

template<>
struct ioctl_trait<binder_feature_set> {
    static constexpr unsigned long  cmd = BINDER_FEATURE_SET;
    static constexpr std::string_view path = "/dev/binder";
};

// 4.3 BINDER_VERSION
// ---------------------------------------

struct binder_version {
    __s32 protocol_version;
};
#define BINDER_VERSION _IOWR('b', 9, struct binder_version)
VISITABLE_STRUCT(binder_version, protocol_version);

template<>
struct ioctl_trait<binder_version> {
    static constexpr unsigned long  cmd = BINDER_VERSION;
    static constexpr std::string_view path = "/dev/binder";
};

// ---------------------------------------
// 5. Types Info
// ---------------------------------------
template<typename... Types>
struct TypesInfoT {
public:
    using VariantType = std::variant<Types...>;
    using FromBufferFunc = std::function<VariantType(const void*, size_t)>;

    struct Info {
        std::string name;
        unsigned long cmd;
        size_t struct_size;
        std::string path;
        FromBufferFunc from_buffer;
    };

    std::map<std::string, Info> infos;

    template<typename T>
    void registerType() {
        std::string_view name = type_name<T>();
        Info& info = infos[std::string(name)];

        info.name = name;
        info.cmd = ioctl_trait<T>::cmd;
        info.struct_size = sizeof(T);
        info.path = ioctl_trait<T>::path;
        info.from_buffer = [=](const void* data, size_t size) -> VariantType {
            if (size < sizeof(T)) {
                throw std::runtime_error("Buffer too small for " + std::string(name));
            }
            T result;
            memcpy(&result, data, sizeof(T));
            return result;
        };
    }

public:
    TypesInfoT() {
        (registerType<Types>(), ...);
    }
};

using TypesInfo = TypesInfoT<winsize, binder_feature_set, binder_version>;
using Info = TypesInfo::Info;
VISITABLE_STRUCT(Info, name, cmd ,struct_size, path);
using VariantType = TypesInfo::VariantType;

TypesInfo typesinfo {};

// ---------------------------------------
// 5. Main
// ---------------------------------------

std::map<std::string, std::string> parse_key_value_args(int argc, char* argv[]) {
    std::map<std::string, std::string> kv_pairs;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            kv_pairs[key] = value;
        }
    }

    return kv_pairs;
}

int main(int argc, char *argv[]) {
    using namespace std;
    // Part1 Parse config
    auto kv_pairs = parse_key_value_args(argc, argv);
    if (kv_pairs.count("@help") > 0 || true) {
        cout << "---------------------------" << endl;
        debug_print("help/", typesinfo.infos);
    }

    // Info *pinfo = &typesinfo.infos.begin()->second;
    Info *pinfo = &typesinfo.infos["winsize"];
    if (kv_pairs.count("@type") > 0) {
        auto it = typesinfo.infos.find(kv_pairs["@type"]);
        if (it != typesinfo.infos.end()) {
            pinfo = &it->second;
        }
    }
    Info info = *pinfo;

    int fd;
    vector<uint8_t> buffer(info.struct_size);

    // We use stdout (or any TTY) as the file descriptor for terminal ioctls
    fd =  open(info.path.c_str(), O_RDONLY);
    cout << "---------------------------" << endl;
    cout << "Using file: " << info.path << endl;
    cout << "Using fd: " << fd << endl;
    cout << "---------------------------" << std::endl;

    int ret = ioctl(fd, info.cmd, buffer.data());
    cout << "ret of ioctl: " << ret << endl;
    if (ret < 0) {
        perror("ioctl(info.cmd) error");
        return EXIT_FAILURE;
    }


    print_buffer(buffer.data(), buffer.size());

    VariantType parsed = info.from_buffer(buffer.data(), buffer.size());

    debug_print("/", parsed);


    // // Modify struct based on command line arguments before ioctl
    // if (!kv_pairs.empty()) {
    //     std::cout << "---------------------------" << std::endl;
    //     std::cout << "Modifying struct with key-value pairs:" << std::endl;
    //     modify_struct_fields(buffer, kv_pairs);
    //     std::cout << "---------------------------" << std::endl;
    //     print_buffer(&buffer, sizeof(buffer));
    //     debug_print("/", buffer);
    // }

    return EXIT_SUCCESS;
}

