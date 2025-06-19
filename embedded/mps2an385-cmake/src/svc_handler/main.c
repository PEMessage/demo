#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL

#if defined(ASSERT)
    #error "macro ASSERT already defined!"
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang: use _Static_assert
    #define ASSERT(expr, msg) _Static_assert(expr, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    // C11 or later: use static_assert
    #define ASSERT(expr, msg) static_assert(expr, msg)
#else
    // Fallback for older compilers: use a hack with typedef and array size
    #define ASSERT(expr, msg) typedef char static_assert_##msg[(expr) ? 1 : -1]
#endif


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
    printf("\n\n-- 5. In SVC, Now we are in SVC_Handler_Main\n");
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

// MPU must align to it's size
// 8 region for max 8 mpu region
char PSP_STACK[2048 * 8] __attribute__((aligned(2048))) = {};


int main() {

    SystemCoreClockUpdate();
    SystemInit();
    extern int stdout_init (void);
    stdout_init();
    // prvUARTInit();

    // 1. Aftre Init complete
    printf("\n\n-- 1. After Init complete\n");
    {
        printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
        get_current_mode();
    }

    printf("\n\n-- 2. Set PSP\n");
    {
        // set theading mode -- psp stack
        __set_PSP((uint32_t)(PSP_STACK+sizeof(PSP_STACK)));
        printf("Control 0x%08"PRIx32"\n"
                "PSP 0x%08"PRIx32"\n"
                "MSP 0x%08"PRIx32"\n",
                __get_CONTROL(), __get_PSP(), __get_MSP());
    }

    printf("\n\n-- 3. Setup dummy MPU(enable full access on FLASH / RAM / UART)\n");
    {
        void CmsisInitMPU(void);
        CmsisInitMPU();
    }

    printf("\n\n-- 4. Switch to Unprivileged!\n");
    {
        __set_CONTROL(0x1);  // 切换到非特权级
        printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
        get_current_mode();
    }

    // See: https://developer.arm.com/documentation/ka004005/latest/
    printf("\n\n-- 5. Raise SVC #0!\n");
    {
        asm volatile ("SVC #0");  // 触发特权切换
    }

    // SVC_Handler_Main will be called here

    printf("\n\n-- 6. After SVC\n");
    {
        printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );
        get_current_mode();
    }


    printf("\n\n-- 7. Print SCS Register\n");
    if (0) {
        void print_SCB_info(void);
        void print_SCnSCB_info(void);
        print_SCnSCB_info();
        print_SCB_info();
    }
    while(1) {
        // printf("Now we are in SVC_Handler_Main\n");
    }

}

void CmsisInitMPU(void) {
    ARM_MPU_Disable();

    // Note: if a symbol variable define in linker script,
    // in C world, should treat that in that a symbol in there(like enum)
    extern void * __ROM_BASE;
    extern void * __ROM_SIZE;
    printf("ROMBASE is %p\n", &__ROM_BASE);
    printf("ROMSIZE is %p\n", &__ROM_SIZE); // and convert it to ARM_MPU_REGION_SIZE
    ARM_MPU_SetRegionEx(
            0UL                           /* Region Number */,
            (uintptr_t)&__ROM_BASE        /* Base Address  */,
            ARM_MPU_RASR(0UL              /* DisableExec */, // ROM region content all code
                                                             //
                ARM_MPU_AP_FULL           /* AccessPermission*/,
                0UL                       /* TypeExtField*/,
                0UL                       /* IsShareable*/,
                0UL                       /* IsCacheable*/,
                0UL                       /* IsBufferable*/,
                0x00UL                    /* SubRegionDisable*/,
                ARM_MPU_REGION_SIZE_256KB /* Size*/)
            );
            // DisableExec change to 1, will cause
            //   Taking exception 3 [Prefetch Abort]
            //   ...with CFSR.IACCVIOL
            //   ...taking pending nonsecure exception 4
            //   Taking exception 3 [Prefetch Abort]
            //   ...with CFSR.IACCVIOL
            //   ...taking pending nonsecure exception 3
            //   Taking exception 3 [Prefetch Abort]
            //   ...with CFSR.IACCVIOL
            //   qemu: fatal: Lockup: can't escalate 3 to HardFault (current priority -1)

    extern void * __RAM_BASE;
    extern void * __RAM_SIZE;
    printf("ROMBASE is %p\n", &__RAM_BASE);
    printf("ROMSIZE is %p\n", &__RAM_SIZE); // and convert it to ARM_MPU_REGION_SIZE
    ARM_MPU_SetRegionEx(
            1UL                           /* Region Number */,
            (uintptr_t)&__RAM_BASE        /* Base Address  */,
            ARM_MPU_RASR(1UL              /* DisableExec */, // RAM region should not content code
                ARM_MPU_AP_FULL           /* AccessPermission*/,
                0UL                       /* TypeExtField*/,
                0UL                       /* IsShareable*/,
                0UL                       /* IsCacheable*/,
                0UL                       /* IsBufferable*/,
                0x00UL                    /* SubRegionDisable*/,
                ARM_MPU_REGION_SIZE_128KB /* Size*/)
            );

    #define UART0_ADDRESS                         ( 0x40004000UL )
    ASSERT(UART0_ADDRESS % (4 * 1024) == 0, "MPU need to aligin to it's size");
    ARM_MPU_SetRegionEx(
            2UL                          /* Region Number */,
            (uintptr_t)UART0_ADDRESS     /* Base Address  */,
            ARM_MPU_RASR(1UL             /* DisableExec */,
                ARM_MPU_AP_FULL          /* AccessPermission*/,
                0UL                      /* TypeExtField*/,
                0UL                      /* IsShareable*/,
                0UL                      /* IsCacheable*/,
                0UL                      /* IsBufferable*/,
                0x00UL                   /* SubRegionDisable*/,
                ARM_MPU_REGION_SIZE_4KB  /* Size*/) // 4K should be already cover all USART register
                                                    //
                                                    // If change to 256K,  cause that(qemu should add `-d int,cpu_reset` ):
                                                    //    Taking exception 4 [Data Abort]
                                                    //    ...with CFSR.DACCVIOL and MMFAR 0x40004004
            );

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk
            | MPU_CTRL_HFNMIENA_Msk
            | MPU_CTRL_ENABLE_Msk);
}



/* Macro to extract bitfield from register */
// #define EXTRACT_BITFIELD(reg, field) ((reg & field##_Msk) >> field##_Pos)
#define EXTRACT_BITFIELD(reg, field) ((typeof(reg))((reg & field##_Msk) >> field##_Pos))

/* Macro to extract field from CMSIS SCS register structure */
#define EXTRACT_FIELD(obj, reg, field) EXTRACT_BITFIELD(obj->reg, obj##_##reg##_##field)

/* Individual register print functions */
void print_CPUID() {
    printf("CPUID Register (0x%08"PRIX32"):\n", SCB->CPUID);
    printf(" |Implementer:          0x%"PRIX32" (ARM == 0x41)\n", EXTRACT_FIELD(SCB, CPUID, IMPLEMENTER));
    printf(" |Variant(rN,Revision): 0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, VARIANT));
    printf(" |Architecture:         0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, ARCHITECTURE));
    printf(" |Part number:          0x%"PRIX32" (Cortex-M3 == C23)\n", EXTRACT_FIELD(SCB, CPUID, PARTNO));
    printf(" |Revision(pN,Patch):   0x%"PRIX32"\n", EXTRACT_FIELD(SCB, CPUID, REVISION));
}
void print_CCR() {
    printf("CCR Register (0x%08"PRIX32"): // Configuration Control Register\n", SCB->CCR);
    printf(" |STKALIGN:        0x%"PRIX32" (Stack alignment)\n", EXTRACT_FIELD(SCB, CCR, STKALIGN));
    printf(" |BFHFNMIGN:       0x%"PRIX32" (BusFault, HardFault NMI ignore)\n", EXTRACT_FIELD(SCB, CCR, BFHFNMIGN));
    printf(" |DIV_0_TRP:       0x%"PRIX32" (Divide by 0 trap)\n", EXTRACT_FIELD(SCB, CCR, DIV_0_TRP));
    printf(" |UNALIGN_TRP:     0x%"PRIX32" (Unaligned access trap) <-- SystemInit will set this, if UNALIGNED_SUPPORT_DISABLE\n",
            EXTRACT_FIELD(SCB, CCR, UNALIGN_TRP));
    printf(" |USERSETMPEND:    0x%"PRIX32" (User level SETMPEND)\n", EXTRACT_FIELD(SCB, CCR, USERSETMPEND));
    printf(" |NONBASETHRDENA:  0x%"PRIX32" (Non-base thread enable)\n", EXTRACT_FIELD(SCB, CCR, NONBASETHRDENA));
}
void print_SCR() {
    // See: https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/system-control-register
    printf("SCR Register (0x%08"PRIX32"): // System Control Register\n", SCB->SCR);
    // Send Event on Pending bit:
    //   0 = only enabled interrupts or events can wake up the processor, disabled interrupts are excluded.
    //   1 = enabled events and all interrupts, including disabled interrupts, can wakeup the processor.
    printf(" |SEVONPEND:      %"PRIu32" (Send Event on Pending)\n", EXTRACT_FIELD(SCB, SCR, SEVONPEND));
    // Controls whether the processor uses sleep or deep sleep as its low power mode:
    //   0 = sleep.
    //   1 = deep sleep
    printf(" |SLEEPDEEP:      %"PRIu32" (Deep Sleep Enable)\n", EXTRACT_FIELD(SCB, SCR, SLEEPDEEP));
    // Indicates sleep-on-exit when returning from Handler mode to Thread mode:
    //   0 = do not sleep when returning to Thread mode.
    //   1 = enter sleep, or deep sleep, on return from an ISR to Thread mode.
    printf(" |SLEEPONEXIT:    %"PRIu32" (Sleep on Exit)\n", EXTRACT_FIELD(SCB, SCR, SLEEPONEXIT));
}

void print_ICSR() {
    printf("ICSR Register (0x%08"PRIX32"):\n", SCB->ICSR);
    /* [31] RW: NMI set-pending bit
     * Write: 1=set pending, Read: 1=NMI pending
     * Highest-priority exception, handler entry clears this bit */
    printf(" |NMIPENDSET:     %"PRIu32" (NMI set-pending bit)\n", EXTRACT_FIELD(SCB, ICSR, NMIPENDSET));
    /* [28] RW: PendSV set-pending bit
     * Write: 1=set pending, Read: 1=PendSV pending
     * Only way to set PendSV pending state */
    printf(" |PENDSVSET:      %"PRIu32" (PendSV set-pending bit)\n", EXTRACT_FIELD(SCB, ICSR, PENDSVSET));
    /* [27] WO: PendSV clear-pending bit
     * Write: 1=clear pending state, Read value undefined */
    printf(" |PENDSVCLR:      %"PRIu32" (PendSV clear-pending bit)\n", EXTRACT_FIELD(SCB, ICSR, PENDSVCLR));
    /* [26] RW: SysTick set-pending bit
     * Write: 1=set pending, Read: 1=SysTick pending */
    printf(" |PENDSTSET:      %"PRIu32" (SysTick set-pending bit)\n", EXTRACT_FIELD(SCB, ICSR, PENDSTSET));
    /* [25] WO: SysTick clear-pending bit
     * Write: 1=clear pending, Read value unknown */
    printf(" |PENDSTCLR:      %"PRIu32" (SysTick clear-pending bit)\n", EXTRACT_FIELD(SCB, ICSR, PENDSTCLR));
    /* [23] RO: Reserved for Debug use
     * Reads-as-zero when not in Debug mode */
    printf(" |ISRPREEMPT:     %"PRIu32" (Reserved for Debug)\n", EXTRACT_FIELD(SCB, ICSR, ISRPREEMPT));
    /* [22] RO: Interrupt pending flag (excludes NMI and Faults)
     * 1=any interrupt pending (except NMI/Faults) */
    printf(" |ISRPENDING:     %"PRIu32" (Any IRQ pending)\n", EXTRACT_FIELD(SCB, ICSR, ISRPENDING));
    /* [17:12] RO: Highest priority pending exception number
     * 0=no pending, Nonzero=exception number
     * Affected by BASEPRI/FAULTMASK but not PRIMASK */
    printf(" |VECTPENDING:    0x%03"PRIX32" (Highest pending IRQ number)\n", EXTRACT_FIELD(SCB, ICSR, VECTPENDING));
    /* [11] RO: Exception preemption status
     * 0=preempted exceptions exist, 1=only current exception active */
    printf(" |RETTOBASE:      %"PRIu32" (No preempted exceptions)\n", EXTRACT_FIELD(SCB, ICSR, RETTOBASE));
    /* [8:0] RO: Currently active exception number
     * 0=Thread mode, Nonzero=active exception number
     * Note: Subtract 16 to get CMSIS IRQ number */
    printf(" |VECTACTIVE:     0x%03"PRIX32" (Active exception number)\n" \
            "    |-- 0=Thread mode, Nonzero=active exception number\n" \
            "    |-- same as IPSR(Special Regs in xPSR), but don't need mrs(Priv) to read\n" ,
            EXTRACT_FIELD(SCB, ICSR, VECTACTIVE));
}

void print_SCB_info() {
    printf("\n===== SCB (System Control Block) =====\n");
    print_CPUID();
    print_ICSR();
    print_CCR();
    print_SCR();
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
