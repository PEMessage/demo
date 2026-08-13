#include <cstdio>

/* Declare the two public interfaces directly; main knows nothing about any mutex */
extern "C" void a_say(const char* msg);
extern "C" void b_say(const char* msg);

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0); /* unbuffer stdout so output is visible before a crash */
    printf("call a_say\n");
    a_say("hello from A");
    printf("call b_say\n");
    b_say("hello from B");
    printf("--- end of main ---\n");
    return 0;
}
