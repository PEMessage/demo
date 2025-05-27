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


// Try do same thing as
// https://stackoverflow.com/questions/18024672/what-registers-are-preserved-through-a-linux-x86-64-function-call
// 这段代码的核心目的是通过Clobber list显式声明哪些寄存器在函数调用中需要被调用者保存。列出的寄存器包括
uint32_t inc(uint32_t i) __attribute__((optimize("O0")));
uint32_t inc(uint32_t i) {
    //  52e:   e92d 4ff0   stmdb   sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    __asm__ __volatile__(
        ""          // 空汇编指令
        : "+m" (i)  // i作为可读写的内存操作数
        :           // 无输入操作数
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", /*"r7",*/ // Clobber list：列出可能被修改的寄存器
          "r8", "r9", "r10", "r11", "r12", "lr",  // 通用寄存器
          "cc", "memory"                          // 条件码和内存屏障
    );
    return i + 1;
    // 542:   e8bd 8ff0   ldmia.w sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
}



// main.c:27:1: error: r7 cannot be used in 'asm' here
//    27 | }
//       | ^
// Maybe same reason? 
//  https://bugs.launchpad.net/gcc-arm-embedded/+bug/1379236
//      The r7 is used as frame pointer register for thumb1 target cortex-m0.
//      It is not allowed to be changed like above ldr instruction when the frame register is actually needed.
//      Above inline assembly code is used in below function:
//  https://github.com/cuberite/cuberite/issues/2387 (not work)
//      Platform
//          ARM (Thumb assembly)
//      What
//          Depending on the GCC compiler options used, you can receive an error:
//          error: r7 cannot be used in asm here
//      Reason
//          The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
//      Solution
//          Add -fomit-frame-pointer to your GCC compiler options.
//          If you have already added -O, -O2, etc you do not need to add -fomit-frame-pointer as the optimization options already include it on most systems by default.



int      main() {

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

    return inc(ret);

}
