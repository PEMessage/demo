#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL
extern int stdout_init (void);



// * 状态字寄存器 （三合一）:
//   * xPSR: 记录 ALU 标志（0 标志，进位标志，负数标志，溢出标志），
//           执行状态，以及当前正服务的中断号
// * 中断屏蔽寄存器 :
//   * PRIMASK: 除能所有的中断——当然了，不可屏蔽中断（NMI）才不甩它呢
//   * FAULTMASK: 除能所有的 fault——NMI 依然不受影响，而且被除能的 faults 会“上访”，见后续章节的叙述
//   * BASEPRI: 除能所有优先级不高于某个具体数值的中断
// * 控制寄存器:
//   * CONTROL: 定义特权状态（见后续章节对特权的叙述），
//              并且决定使用哪一个堆栈指针
//     * nPRIV (Bit 0): 特权级别控制 - 0: 主任务模式为特权级（privileged），1: 主任务模式为非特权级（unprivileged）
//     * SPSEL (Bit 1): 堆栈指针选择 - 0: 使用主堆栈指针 MSP，1: 使用进程堆栈指针 PSP
//     * FPCA (Bit 2): 浮点上下文保存激活标志 - 1: 当前线程使用了浮点寄存器，需要保存浮点状态
//     * [31:2]: 保留位，必须写为 0
uint32_t get_current_mode(void) {
    uint32_t ipsr = __get_IPSR();
    if (ipsr == 0) {
        printf("Your are runing Threading Mode\n");
        return 0; // Thread Mode
    } else {
        printf("Your are runing Handler Mode: %"PRIu32"\n", ipsr); // ips - 16 to get IRQ number
        return 1; // Handler Mode
    }
}
// SVC 异常处理函数
// See: https://developer.arm.com/documentation/ka004005/latest/
void  SVC_Handler() {
    // Note: we should call anything before following ASM Parse, it will break something
    __asm volatile (
            ".global SVC_Handler_Main\n"
            "tst lr, #4\n"       // 检查 EXC_RETURN 的栈帧类型
            "ite eq\n"
            "mrseq r0, msp\n"    // 使用 MSP
            "mrsne r0, psp\n"    // 使用 PSP
            "b SVC_Handler_Main\n"
            );
}

void SVC_Handler_Main( unsigned int *svc_args )
{
    unsigned int svc_number;

    /*
     * Stack contains:
     * r0, r1, r2, r3, r12, r14(LR), the return address and xPSR
     * First argument (r0) is svc_args[0]
     */
    svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
    // TODO: why printf doesn't work?
    printf("\n\n-- 4. In SVC, Now we are in SVC_Handler_Main\n");
    get_current_mode();
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
    switch( svc_number )
    {
        case 0:  /* EnablePrivilegedMode */
            __set_CONTROL( __get_CONTROL( ) & ~CONTROL_nPRIV_Msk ) ;
            break;
        default:    /* unknown SVC */
            break;
    }
}


int main() {

    SystemCoreClockUpdate();
    stdout_init();
    // prvUARTInit();

    // 1. Aftre Init complete
    printf("\n\n-- 1. After Init complete\n");
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
    get_current_mode();

    printf("\n\n-- 2. Switch to privileged!\n");
    __set_CONTROL(0x1);  // 切换到非特权级
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
    get_current_mode();

    // See: https://developer.arm.com/documentation/ka004005/latest/
    printf("\n\n-- 3. Raise SVC #0!\n");
    asm volatile ("SVC #0");  // 触发特权切换

    printf("\n\n-- 5. After SVC\n");
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
    get_current_mode();

    printf("\n\n-- 6. Print SCS Register\n");
    void print_SCB_info(void);
    void print_SCnSCB_info(void);
    print_SCnSCB_info();
    print_SCB_info();
    while(1) {
        // printf("Now we are in SVC_Handler_Main\n");
    }

}

/* Macro to extract bitfield from register */
// #define EXTRACT_BITFIELD(reg, field) ((reg & field##_Msk) >> field##_Pos)
#define EXTRACT_BITFIELD(reg, field) ((typeof(reg))((reg & field##_Msk) >> field##_Pos))

/* Macro to extract field from CMSIS SCS register structure */
#define EXTRACT_FIELD(obj, reg, field) EXTRACT_BITFIELD(obj->reg, obj##_##reg##_##field)

/* Individual register print functions */
void print_CPUID() {
    printf("CPUID Register (0x%08"PRIX32"):\n", SCB->CPUID);
    printf(" |Implementer:    0x%"PRIX32" (ARM)\n", EXTRACT_FIELD(SCB, CPUID, IMPLEMENTER));
    printf(" |Variant:        0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, VARIANT));
    printf(" |Architecture:   0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, ARCHITECTURE));
    printf(" |Part number:    0x%"PRIX32" (Cortex-M3)\n", EXTRACT_FIELD(SCB, CPUID, PARTNO));
    printf(" |Revision:       0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, REVISION));
}

void print_SCB_info() {
    printf("\n===== SCB (System Control Block) =====\n");
    print_CPUID();
}

/* 打印 ICTR 寄存器信息 */
void print_ICTR() {
    printf("ICTR Register (0x%08"PRIX32"): // Interrupt Controller Type Register \n", SCnSCB->ICTR);
    printf(" |Interrupt lines supported: %"PRIu32" (32 * (%"PRIu32" + 1))\n",
           32 * (EXTRACT_FIELD(SCnSCB, ICTR, INTLINESNUM) + 1),
           EXTRACT_FIELD(SCnSCB, ICTR, INTLINESNUM));
}

/* 打印 ACTLR 寄存器信息 Vendor Used it */
void print_ACTLR() {
    printf("ACTLR Register (0x%08"PRIX32"): // Auxiliary Control Register \n", SCnSCB->ACTLR);
}

/* 主打印函数 */
void print_SCnSCB_info() {
    printf("\n===== SCnSCB (System Control not in SCB) =====\n");
    print_ICTR();
    print_ACTLR();
    // print_CPPWR();
}
