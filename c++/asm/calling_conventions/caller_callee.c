
// Helper macros for syscalls
#if defined(__x86_64__)
    #define SYS_exit 60
    static inline long __syscall1(long n, long a1) {
        unsigned long ret;
        __asm__ __volatile__ (
            "syscall"
            : "=a"(ret)
            : "a"(n), "D"(a1)
            : "rcx", "r11", "memory"
        );
        return ret;
    }
#elif defined(__i386__)
    #define SYS_exit 1
    static inline long __syscall1(long n, long a1) {
        unsigned long ret;
        __asm__ __volatile__ (
            "int $0x80"
            : "=a"(ret)
            : "a"(n), "b"(a1)
            : "memory"
        );
        return ret;
    }
#elif defined(__aarch64__)
    #define SYS_exit 93
    static inline long __syscall1(long n, long a) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        __asm__ __volatile__ (
            "svc #0"
            : "=r"(x0)
            : "r"(x8), "0"(x0)
            : "memory"
        );
        return x0;
    }
#elif defined(__riscv)
    #define SYS_exit 93
    static inline long __syscall1(long n, long a1) {
        register long a0 __asm__("a0") = a1;
        register long a7 __asm__("a7") = n;
        __asm__ __volatile__ (
            "ecall"
            : "+r"(a0)
            : "r"(a7)
            : "memory"
        );
        return a0;
    }
#else

    #error "Unsupported architecture"
#endif


int add(int a, int b, int c, int d,
        int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

void _start() {
    int ret = add(1, 2, 3, 4, 5, 6, 7, 8);
    __syscall1(SYS_exit, 33);  // Exit with status 0
}


