#include <unistd.h>

// gcc -static -o hello --entry main 
#define STR "Hello linux\n"
int main(int argc, char *argv[])
{
    write(1, STR, sizeof(STR));
    while(1) ;
    _exit(0x04);
}
