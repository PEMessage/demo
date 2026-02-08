#include <inttypes.h>
#define START "_start"

__asm__(
        ".text \n"
        ".global " START " \n"
        START ": \n"
        "	xor %rbp,%rbp \n"
        "	mov %rsp,%rdi \n"
        ".weak _DYNAMIC \n"
        ".hidden _DYNAMIC \n"
        "	lea _DYNAMIC(%rip),%rsi \n"
        "	andq $-16,%rsp \n"
        "	call " START "_c \n"
       );

void _start_c(long *p)
{
    int argc = p[0];
    p++;

    char **argv = (void *)(p);
    p+= argc;
    p++; // argv also is a null terminate one

    // See:
    // https://gist.github.com/PEMessage/dcb12d6189dca3316b93b5ef9333bfb5#statically-linked-executable-files-1
    // https://camo.githubusercontent.com/1bb05c6ac33c880385ae626ef076dbbeb04d27beed407ee4c369880d5246ccad/68747470733a2f2f692e696d6775722e636f6d2f5a79364a7332302e706e67
    char **env = (void *)(p);
    while(*p != 0) { p++; }
    int enc = ((void *)p - (void *)env) / sizeof(void *);
    p++; // skip null terminate

    int main_me(int argc, char **argv, int enc, char **env);
    main_me(argc, argv, enc, env);
}


// -------------------------------
#define SYS_write 1
#define SYS_exit  60

static long syscall(long n, long a1, long a2, long a3) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3)
            : "rcx", "r11", "memory");
    return ret;
}

static void write_str(const char *s) {
    long len = 0;
    while (s[len]) len++;
    syscall(SYS_write, 1, (long)s, len);
}

static void exit(int status) {
    syscall(SYS_exit, status, 0, 0);
}


int main_me(int argc, char **argv, int enc, char **env) {
    write_str("Hello from minimal startup!\n");

    write_str("-------\n");

    for (int i = 0; i < enc ; i++) {
        write_str("env: ");
        write_str(env[i]);
        write_str("\n");
    }

    write_str("-------\n");

    for (int i = 0; i < argc ; i++) {
        write_str("arg: ");
        write_str(argv[i]);
        write_str("\n");
    }

    write_str("-------\n");

    write_str("Done\n");
    exit(0);
}
