#include <stdio.h>



#define mod16_error(in) \
    do {\
        printf("The %d mod 16 is: %d\n", in, in % 16); \
    }while(0)

#define mod16_correct(in) \
    do {\
        printf("The %d mod 16 is: %d\n", in, (in) % 16); \
    }while(0)

int main(int argc, char *argv[])
{
    mod16_error(32); // 0
    mod16_correct(32); // 0
    mod16_error(30 + 2); // NOTE: the output is 32 , beacause 30 + 2 % 16 == 30 + (2 % 16) == 32
    mod16_correct(30 + 2); // 0
    return 0;
}
