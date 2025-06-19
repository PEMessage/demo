#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 采用"异或链"(XOR chain)技术进行流程验证：
//   每个步骤都有一个唯一的魔数(魔法数字)作为"步骤值"(STEP_VALUE)
//   通过连续异或操作验证流程完整性
//   如果任何一步的验证失败，将调用Error_Handler()
//   See: Projects/B-U585I-IOT02A/Applications/SBSFU/SBSFU_Boot/Inc/boot_hal_flowcontrol.h


/* 定义错误处理函数 */
void Error_Handler(const char* step) {
    printf("Flow Control Error at step: %s\n", step);
    exit(EXIT_FAILURE);
}

/* 定义流程步骤值 */
#define INIT_VALUE        0x12345678U
#define STEP_1_ENABLE     0x87654321U
#define STEP_2_ENABLE     0xABCDEF12U
#define STEP_3_ENABLE     0x34567890U

#define STEP_1_CHECK      0x11111111U
#define STEP_2_CHECK      0x22222222U
#define STEP_3_CHECK      0x33333333U

/* 计算预期的控制值 */
#define CTRL_AFTER_STEP_1 (INIT_VALUE ^ STEP_1_ENABLE)
#define CTRL_AFTER_STEP_2 (CTRL_AFTER_STEP_1 ^ STEP_2_ENABLE)
#define CTRL_AFTER_STEP_3 (CTRL_AFTER_STEP_2 ^ STEP_3_ENABLE)

/* 流程控制宏 */
#define FLOW_STEP(CURRENT, STEP, EXPECTED) \
    do { \
        printf("Executing step: " #STEP "\n"); \
        (CURRENT) ^= (STEP); \
        if ((CURRENT) != (EXPECTED)) { \
            Error_Handler(#STEP); \
        } \
        printf("Flow control value after step: 0x%08X\n\n", CURRENT); \
    } while(0)

#define FLOW_CHECK(CURRENT, EXPECTED) \
    do { \
        printf("Checking flow at: " #EXPECTED "\n"); \
        if ((CURRENT) != (EXPECTED)) { \
            Error_Handler("Check failed"); \
        } \
        printf("Check passed! Value: 0x%08X\n\n", CURRENT); \
    } while(0)

int main() {
    uint32_t uFlowValue = INIT_VALUE;
    
    printf("=== XOR Chain Flow Control Demo ===\n");
    printf("Initial flow value: 0x%08X\n\n", uFlowValue);
    
    /* 第一阶段：执行步骤 */
    FLOW_STEP(uFlowValue, STEP_1_ENABLE, CTRL_AFTER_STEP_1);
    FLOW_STEP(uFlowValue, STEP_2_ENABLE, CTRL_AFTER_STEP_2);
    FLOW_STEP(uFlowValue, STEP_3_ENABLE, CTRL_AFTER_STEP_3);
    
    /* 第二阶段：检查步骤 */
    printf("=== XOR Chain Flow Control reinit, skip step2 ===\n");
    uFlowValue = INIT_VALUE;
    
    
    FLOW_STEP(uFlowValue, STEP_1_ENABLE, CTRL_AFTER_STEP_1);
    printf("Skip step2 to trigger error, (for example, we meet fault_injection)\n\n");
    // FLOW_STEP(uFlowValue, STEP_2_ENABLE, CTRL_AFTER_STEP_2);
    FLOW_STEP(uFlowValue, STEP_3_ENABLE, CTRL_AFTER_STEP_3);
    
    printf("You should never get here!");
    return 0;
}
