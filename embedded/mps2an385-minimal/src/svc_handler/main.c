#include <stdio.h>
#include <stdint.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL
extern int stdout_init (void);

// SVC 异常处理函数
// See: https://developer.arm.com/documentation/ka004005/latest/
void  SVC_Handler() {
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
  * r0, r1, r2, r3, r12, r14, the return address and xPSR
  * First argument (r0) is svc_args[0]
  */
  svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
  // TODO: why printf doesn't work?
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

    stdout_init();

    __set_CONTROL(0x1);  // 切换到非特权级
    printf("Switch to privileged!\n");
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );

    // See: https://developer.arm.com/documentation/ka004005/latest/
    printf("Raise SVC #0!\n");
    asm volatile ("SVC #0");  // 触发特权切换
    printf("Is privileged : %d\n", (__get_CONTROL() & 0x1) == 0 );

}
