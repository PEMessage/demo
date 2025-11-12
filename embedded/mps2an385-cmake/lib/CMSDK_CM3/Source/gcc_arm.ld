/******************************************************************************
 * @file     gcc_arm.ld
 * @brief    GNU Linker Script for Cortex-M based device
 ******************************************************************************/

/*
; -------- <<< Use Configuration Wizard in Context Menu >>> -------------------
 */

/*---------------------- Flash Configuration ----------------------------------
; <h> Flash Configuration
;   <o0> Flash Base Address <0x0-0xFFFFFFFF:8>
;   <o1> Flash Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
  -----------------------------------------------------------------------------*/
__ROM_BASE = 0x00000000;
__ROM_SIZE = 0x00400000;

/*--------------------- Embedded RAM Configuration ----------------------------
; <h> RAM Configuration
;   <o0> RAM Base Address    <0x0-0xFFFFFFFF:8>
;   <o1> RAM Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
 -----------------------------------------------------------------------------*/
__RAM_BASE = 0x20000000;
__RAM_SIZE = 0x00020000;

/*--------------------- Stack / Heap Configuration ----------------------------
; <h> Stack / Heap Configuration
;   <o0> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;   <o1> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
  -----------------------------------------------------------------------------*/
__STACK_SIZE = 0x00000400;
__HEAP_SIZE  = 0x00000C00;

/*
; -------------------- <<< end of configuration section >>> -------------------
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = __ROM_BASE, LENGTH = __ROM_SIZE
  RAM   (rwx) : ORIGIN = __RAM_BASE, LENGTH = __RAM_SIZE
}

__privileged_functions_region_size__  = 64K; /* 64000 bytes */
__privileged_data_region_size__       = 32K; /* 32000 bytes */

__FLASH_segment_start__               = ORIGIN( FLASH );
__FLASH_segment_end__                 = __FLASH_segment_start__ + LENGTH( FLASH );

__SRAM_segment_start__      = ORIGIN( RAM );
__SRAM_segment_end__        = __SRAM_segment_start__ + LENGTH( RAM );

__privileged_data_start__   = __SRAM_segment_start__;
__privileged_data_end__     = __SRAM_segment_start__ + __privileged_data_region_size__;

__privileged_functions_start__        = __FLASH_segment_start__;
__privileged_functions_end__          = __FLASH_segment_start__ + __privileged_functions_region_size__;

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __process_stack_start (SVC_HANDLER_DEMO only)
 *   __ProcessStackLimit (SVC_HANDLER_DEMO only)
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
  .privileged_functions :
  {
    . = ALIGN(4);
    KEEP(*(.vectors))
    . = ALIGN(4);
    *(privileged_functions)
    . = ALIGN(4);
    FILL(0xDEAD);
    /* Ensure that un-privileged code is placed after the region reserved                                                                                                                                         4          * for privileged kernel code. */
    /* Note that dot (.) actually refers to the byte offset from the start
        * of the current section (.privileged_functions in this case). As a
        * result, setting dot (.) to a value sets the size of the section. */
    . = __privileged_functions_region_size__;
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    __syscalls_flash_start__ = .;
    *(freertos_system_calls)
    __syscalls_flash_end__ = .;
    . = ALIGN(4);
    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    *(.rodata*)

    KEEP(*(.eh_frame*))
  } > FLASH

  /*
   * SG veneers:
   * All SG veneers are placed in the special output section .gnu.sgstubs. Its start address
   * must be set, either with the command line option ‘--section-start’ or in a linker script,
   * to indicate where to place these veneers in memory.
   */
/*
  .gnu.sgstubs :
  {
    . = ALIGN(32);
  } > FLASH
*/
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > FLASH
  __exidx_end = .;

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;

    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)

    /* Add each additional data section here */
/*
    LONG (__etext2)
    LONG (__data2_start__)
    LONG ((__data2_end__ - __data2_start__) / 4)
*/
    __copy_table_end__ = .;
  } > FLASH

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    /* Add each additional bss section here */
/*
    LONG (__bss2_start__)
    LONG ((__bss2_end__ - __bss2_start__) / 4)
*/
    __zero_table_end__ = .;
  } > FLASH

  /**
   * Location counter can end up 2byte aligned with narrow Thumb code but
   * __etext is assumed by startup code to be the LMA of a section in RAM
   * which must be 4byte aligned
   */
  __etext = ALIGN (4);

  .data : AT (__etext)
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* All data end */
    __data_end__ = .;

  } > RAM

  /*
   * Secondary data section, optional
   *
   * Remember to add each additional data section
   * to the .copy.table above to asure proper
   * initialization during startup.
   */
/*
  __etext2 = ALIGN (4);

  .data2 : AT (__etext2)
  {
    . = ALIGN(4);
    __data2_start__ = .;
    *(.data2)
    *(.data2.*)
    . = ALIGN(4);
    __data2_end__ = .;

  } > RAM2
*/

  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM AT > RAM

  /*
   * Secondary bss section, optional
   *
   * Remember to add each additional bss section
   * to the .zero.table above to asure proper
   * initialization during startup.
   */
/*
  .bss2 :
  {
    . = ALIGN(4);
    __bss2_start__ = .;
    *(.bss2)
    *(.bss2.*)
    . = ALIGN(4);
    __bss2_end__ = .;
  } > RAM2 AT > RAM2
*/

  .heap (COPY) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > RAM

#ifdef SVC_HANDLER_DEMO
  .process_stack (COPY) :
  {
    __process_stack_start = .;
    *(.process_stack)
    __process_stack_end = .;
    __ProcessStackLimit = .;
  } > RAM
#endif

  .stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > RAM
  PROVIDE(__stack = __StackTop);

#ifdef SVC_HANDLER_DEMO
  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __ProcessStackLimit, "region RAM overflowed with stack")
#else
  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
#endif
}
