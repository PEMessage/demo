# main.s

.global _reset
_reset:
	# Set up stack pointer
	LDR X2, =stack_top
	MOV SP, X2
	# Magic number
	MOV X13, #0x1337
	# Loop endlessly
	B . 

