#include <stdio.h>
#include <stdint.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL
extern int stdout_init (void);

int add(int a, int b, int c, int d, \
        int e, int f, int g, int h) __attribute__((optimize("O0")));

int add(int a, int b, int c, int d,
        int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main() {

    int ret = add(1, 2, 3, 4, 5, 6, 7, 8);
    // 使用16位的movs指令（而不是32位的mov）
    // strd是32位指令，用于同时存储两个寄存器到内存
    // 530:   2307       movs    r3, #7         ; r3 = 7 (第7个参数)
    // 532:   2208       movs    r2, #8         ; r2 = 8 (第8个参数)
    // 536:   e9cd 3202  strd    r3, r2, [sp, #8] ; 将r3(7)和r2(8)存储到sp+8和sp+12
    // 53a:   2305       movs    r3, #5         ; r3 = 5 (第5个参数)
    // 534:   2106       movs    r1, #6         ; r1 = 6 (第6个参数)
    // 53e:   e9cd 3100  strd    r3, r1, [sp]   ; 将r3(5)和r1(6)存储到sp和sp+4
    //
    // ---------- Stack ----------
    // sp+12: 8
    // sp+8:  7
    // sp+4:  6
    // sp+0:  5
    // ---------- Stack ----------
    //
    // 在ARM架构中，参数传递遵循AAPCS（ARM Architecture Procedure Call Standard）规则：
    // 前4个参数通过寄存器R0-R3传递
    // 剩余参数通过栈传递
    // 542:   2001       movs    r0, #1         ; r0 = 1 (第1个参数)
    // 546:   2102       movs    r1, #2         ; r1 = 2 (第2个参数)
    // 53c:   2203       movs    r2, #3         ; r2 = 3 (第3个参数)
    // 544:   2304       movs    r3, #4         ; r3 = 4 (第4个参数)
    //
    // 548:	f7ff ffd6 	bl	4f8 <add>

    return ret;

}
