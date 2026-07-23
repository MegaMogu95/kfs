.set KER_CODE,	0x00cf9a000000ffff
.set KER_DATA,	0x00cf92000000ffff
.set KER_STACK,	0x00cf92000000ffff
.set USR_CODE,	0x00cffa000000ffff
.set USR_DATA,	0x00cff2000000ffff
.set USR_STACK,	0x00cff2000000ffff

.section .data
gdt_start:
	.quad	0
	.quad	KER_CODE
	.quad	KER_DATA
	.quad	KER_STACK
	.quad	USR_CODE
	.quad	USR_DATA
	.quad	USR_STACK
gdt_end:

gdtr:
	.word	gdt_end - gdt_start - 1
	.long	0x800

.section .text

.global gdt_init

gdt_init:
	movl	$gdt_start, %esi
	movl	$0x800, %edi
	movl	$14, %ecx
	rep		movsl
	lgdt	gdtr
	movw		$0x10, %ax
	movw		%ax, %ds
	movw		%ax, %es
	movw		%ax, %fs
	movw		%ax, %gs
	movw		$0x18, %ax
	movw		%ax, %ss
	ljmp		$0x08, $1f
	1:
	ret

