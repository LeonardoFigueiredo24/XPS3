ENTRY(_start)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS
{
	. = 0x10000;
	.text : { *(.text .text.*) } :text
	.rodata : { *(.rodata .rodata.*) } :text
	. = ALIGN(0x10000);
	.data : { *(.opd_entry) *(.data .data.*) } :data
	.got : { *(.got) *(.toc) } :data
	.bss : { *(.bss .bss.* COMMON) } :data
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
