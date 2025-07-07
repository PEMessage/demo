
#include <stdio.h>


#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)


int main() {
    printf("Current file name: %s\n", __FILE_NAME__);
    #if (__GNUC__ >= 12) || defined(__clang__)
    // See: 
    // Since gcc-12, https://www.bensyz.com/blogs/gcc_support_filename_macro/
    // https://www.bensyz.com/blogs/gcc_support_filename_macro/
    printf("Full file path: %s\n", __FILE__);
    #endif
    return 0;
}
