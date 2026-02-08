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
	char **argv = (void *)(p+1);

    int main_me(int argc, char **argv);
	main_me(argc, argv);
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


int main_me(int argc, char **argv) {
    write_str("Hello from minimal startup!\n");
    for (int i = 0; i < argc ; i++) {
        write_str("arg: ");
        write_str(argv[i]);
    }

    while(1); // without while loop, might be exit without print, is that normal?
    return 0;
}
