[BITS 32]
[global _start]

MB_ALIGN		equ  1<<0
MB_MEMINFO		equ  1<<1
MB_VIDINFO		equ  1<<2
MB_AOUT			equ  1<<16

MULTIBOOT_HEADER_MAGIC   equ  0x1BADB002
MULTIBOOT_HEADER_FLAGS   equ  MB_ALIGN | MB_MEMINFO | MB_VIDINFO
CHECKSUM                 equ  -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .multiboot

multiboot_header:
	dd	MULTIBOOT_HEADER_MAGIC
	dd	MULTIBOOT_HEADER_FLAGS
	dd	CHECKSUM

	dd	0
	dd	0
	dd	0
	dd	0
	dd	0
	
	dd	0
	dd	1024
	dd	768
	dd	32

section .text
extern kernel_main

_start:
	;mov	esp, KERNEL_STACK	;Setup the stack
	;push	0					;Reset EFLAGS
	;popf
	
	push	ebx					;Multiboot info 
	push	eax					;Magic
	call	kernel_main

	cli
	hlt