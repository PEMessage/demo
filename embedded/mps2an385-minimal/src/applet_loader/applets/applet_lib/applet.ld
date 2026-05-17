/* Minimal applet linker script for mps2an385 */

MEMORY
{
  /* NOTICE: if you change this, all applet should be update(will not rebuild automatic) */
  ram (rwx) : ORIGIN = 0x40000000, LENGTH = 32K
}

SECTIONS
{
  .applet_header : {
    KEEP(*(.applet_header))
  } > ram
  
  .text : {
    *(.text*)
    *(.rodata*)
  } > ram
  
  .data : {
    *(.data*)
  } > ram

  .got : {
    *(.got)
    *(.got.plt)
  } > ram

  __got_start = ADDR(.got);
  __got_end = ADDR(.got) + SIZEOF(.got);

  .bss : {
    *(.bss*)
    *(COMMON)
  } > ram

  __bss_start = ADDR(.bss);
  __bss_end = ADDR(.bss) + SIZEOF(.bss);
}
