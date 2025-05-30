/*-----------------------------------------------------------------------------
 * Name:    stdout_USART.c
 * Purpose: STDOUT USART Template
 * Rev.:    1.0.0
 *-----------------------------------------------------------------------------*/
 
/* Copyright (c) 2013 - 2015 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/
 
#include "Driver_USART.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

 
//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------
 
// <h>STDOUT USART Interface
 
//   <o>Connect to hardware via Driver_USART# <0-255>
//   <i>Select driver control block for USART interface
#define USART_DRV_NUM           0
 
//   <o>Baudrate
#define USART_BAUDRATE          115200
 
// </h>
 
 
#define _USART_Driver_(n)  Driver_USART##n
#define  USART_Driver_(n) _USART_Driver_(n)
 
extern ARM_DRIVER_USART  USART_Driver_(USART_DRV_NUM);
#define ptrUSART       (&USART_Driver_(USART_DRV_NUM))
 
 
/**
  Initialize stdout
 
  \return          0 on success, or -1 on error.
*/
int stdout_init (void) {
  int32_t status;
 
  status = ptrUSART->Initialize(NULL);
  if (status != ARM_DRIVER_OK) return (-1);
 
  status = ptrUSART->PowerControl(ARM_POWER_FULL);
  if (status != ARM_DRIVER_OK) return (-1);
 
  status = ptrUSART->Control(ARM_USART_MODE_ASYNCHRONOUS |
                             ARM_USART_DATA_BITS_8       |
                             ARM_USART_PARITY_NONE       |
                             ARM_USART_STOP_BITS_1       |
                             ARM_USART_FLOW_CONTROL_NONE,
                             USART_BAUDRATE);
  if (status != ARM_DRIVER_OK) return (-1);

  status = ptrUSART->Control(ARM_USART_CONTROL_TX, 1);
  if (status != ARM_DRIVER_OK) return (-1);

  return (0);
}
 
 
/**
  Put a character to the stdout
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch) {
  // Note: this function has bug under QEMU SVC_Handler
  uint8_t buf[1];
 
  buf[0] = ch;
  if (ptrUSART->Send(buf, 1) != ARM_DRIVER_OK) {
    return (-1);
  }
  while (ptrUSART->GetTxCount() != 1);
  return (ch);
}


// Required for printf
// Old implement has bug?
// int _write(int file, char *ptr, int len) {
//     int i;

//     if (file == 2 || file == 1) {
//         for (i = 0; i < len; i++) {
//             if (stdout_putchar(ptr[i]) != ptr[i]) {
//                 return -1;
//             }
//         }
//         return i;
//     }

//     return -1;
// }

// Implement from FreeRTOS/Demo/CORTEX_MPS2_QEMU_IAR_GCC/main.c
static void prvUARTInit( void )
{
    #define UART0_ADDRESS                         ( 0x40004000UL )
    #define UART0_DATA                            ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 0UL ) ) ) )
    #define UART0_STATE                           ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 4UL ) ) ) )
    #define UART0_CTRL                            ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 8UL ) ) ) )
    #define UART0_BAUDDIV                         ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 16UL ) ) ) )
    #define TX_BUFFER_MASK                        ( 1UL )
    UART0_BAUDDIV = 16;
    UART0_CTRL = 1;
}

// Implement from FreeRTOS/Demo/CORTEX_MPS2_QEMU_IAR_GCC/main.c
int _write( int iFile,
             char * pcString,
             int iStringLength )
{
    int iNextChar;

    /* Avoid compiler warnings about unused parameters. */
    ( void ) iFile;

    /* Output the formatted string to the UART. */
    for( iNextChar = 0; iNextChar < iStringLength; iNextChar++ )
    {
        while( ( UART0_STATE & TX_BUFFER_MASK ) != 0 )
        {
        }

        UART0_DATA = *pcString;
        // stdout_putchar( *pcString ); // This function work abnormal under QEMU SVC_Handler
        pcString++;
    }

    return iStringLength;
}
