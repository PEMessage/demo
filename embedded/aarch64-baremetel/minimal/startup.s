# main.s

.global _reset
_reset:
	# Set up stack pointer
	ldr x2, =stack_top
	mov sp, x2
	# Send '!'
    ldr x10, uartdr
loop_send:
    mov w9, '!'
    strb w9, [x10]
	b loop_send
loop:
    b loop

uartdr: 
	.quad 0x9000000

