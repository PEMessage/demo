#include <stddef.h>
#include <stdio.h>

size_t align_up(size_t len, size_t align) {
    return (len + align - 1) / align * align;
}

#define ALIGN_UP(len, align) (((len) + (align) - 1) / (align) * (align))
#define ALIGN_DOWN(len, align) ((len) / (align) * (align))



int main(int argc, char *argv[])
{
    int align = 8;
    for (int len = 0 ; len < 32 ; len++) {
        printf(
                "len: %d, aligned up: %d, aligned down: %d\n",
                len,
                ALIGN_UP(len, align),
                ALIGN_DOWN(len, align)
              );
    }
}
