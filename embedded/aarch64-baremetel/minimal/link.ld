/* linker.ld */
SECTIONS {
	. = 0x40100000;
	.text : { *(.text) }
	. = ALIGN(8);
	. = . + 0x1000; 
	stack_top = .;
}
