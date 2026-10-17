# Entry for the bench ELFs: RPCS3 calls e_entry as a 32-bit OPD (code address, TOC).
# r1 holds the stack, r2 the TOC from the OPD; we set r2 ourselves anyway.
	.section .opd_entry,"aw"
	.globl _start
	.p2align 3
_start:
	.long _start_code
	.long 0

	.text
	.globl _start_code
	.p2align 4
_start_code:
	lis 2, .TOC.@ha
	addi 2, 2, .TOC.@l
	stdu 1, -128(1)
	bl bench_main
	nop
	# _sys_process_exit(status, 0, 0)
	li 4, 0
	li 5, 0
	li 11, 22
	sc
1:	b 1b

# Thread entry trampoline: a PPU thread entry OPD points here; r3 = arg (thread_ctx*).
	.globl _thread_code
	.p2align 4
_thread_code:
	lis 2, .TOC.@ha
	addi 2, 2, .TOC.@l
	stdu 1, -128(1)
	bl bench_thread
	nop
	# _sys_ppu_thread_exit(r3)
	li 11, 41
	sc
1:	b 1b

	.section .opd_entry,"aw"
	.globl _thread_opd
	.p2align 3
_thread_opd:
	.long _thread_code
	.long 0

# SPU kernels (flat LS image from spu_asm.py), 128-byte aligned for the segment copy.
	.section .rodata
	.globl spu_image
	.globl spu_image_end
	.p2align 7
spu_image:
	.incbin "spu_kernels.bin"
spu_image_end:
