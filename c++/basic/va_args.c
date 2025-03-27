
#include <stdio.h>
#include <stdarg.h>
void mylog(const char *perfix, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    printf("[%s]: ", perfix);
    // Not correct
    //      printf(fmt, ap);
    //      >
    //      [INFO]: Hello world
    //      [INFO]: Hello 891590864
    // Correct
    //      When using variable arguments with printf,
    //      you should use vprintf instead of printf
    //      to handle the va_list properly
    vprintf(fmt, ap);  // Changed from printf to vprintf
    printf("\n");
    va_end(ap);
}

int main(int argc, char *argv[])
{
    mylog("INFO", "Hello world");
    mylog("INFO", "Hello %d", 123);
    return 0;
}
