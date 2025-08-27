#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <iostream>
#include "visit_struct/visit_struct.hpp"

#include <string_view>
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

VISITABLE_STRUCT(winsize, ws_row, ws_col, ws_xpixel, ws_ypixel);

#define IOCTL_CMD     TIOCGWINSZ
typedef struct winsize IOCTL_STRUCT;
#define IOCTL_FILE "/dev/tty"


void print_buffer(void *ptr, size_t len) {
    unsigned char *addr = (unsigned char *)ptr;

    // Print the pointer address in hex (byte by byte)
    printf("Content: \n");
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", ((unsigned char*)&ptr)[i]);
        if (i % 8 == 8 -1) {
            printf("\n");
        }
    }
    printf("\n");
}


template <class T>
void meta_print(const char* name, const T& value) {
    if constexpr(std::is_same_v<T, char> || std::is_same_v<T, unsigned char>) {
        std::cout << name << ": " << (int)value;
    } else {
        std::cout << name << ": " << value;
    }
}

template<typename T>
struct is_char_array : std::false_type {};

template<std::size_t N>
struct is_char_array<char[N]> : std::true_type {};

template<std::size_t N>
struct is_char_array<const char[N]> : std::true_type {};

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
    } else {
        std::cout << INDENT;
        meta_print(name, value);
        std::cout << std::endl;
    }
}

int main() {
    int fd;
    IOCTL_STRUCT buffer {};

    // We use stdout (or any TTY) as the file descriptor for terminal ioctls
    printf("Using file: %s\n", IOCTL_FILE);
    fd =  open(IOCTL_FILE, O_RDONLY);

    printf("Using file descriptor: %d\n", fd);

    // Perform ioctl to get window size
    int ret = ioctl(fd, IOCTL_CMD, &buffer);
    printf("ioctl return value: %d\n", ret);

    if (ret < 0) {
        perror("ioctl IOCTL_CMD failed");
        return EXIT_FAILURE;
    }

    print_buffer(&buffer, sizeof(buffer));
    debug_print("/", buffer);

    return EXIT_SUCCESS;
}
