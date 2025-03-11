

#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[])
{

    uint8_t a[1024] = {} ;
    uint8_t *start = NULL;
    uint32_t len = 0 ;

    printf("%p\n", a); // 0x7ffca61e32f0

    printf("%p\n", a + 1);     // 0x7ffca61e32f1 
    printf("%p\n", &a[0] + 1); // 0x7ffca61e32f1

    // 在表达式中，数组名会退化为指向其第一个元素的指针。
    printf("%p\n", &a[0] + sizeof(a) - len); // 0x7ffca61e36f0
    printf("%p\n", a     + sizeof(a) - len); // 0x7ffca61e36f0

    return 0;
}
