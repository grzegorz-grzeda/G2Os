MEMORY
{
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
	ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
}
__stack = ORIGIN(RAM) + LENGTH(RAM);
__main_stack_size = 1024;
__heap_begin = __init_end;
__heap_end = __stack - __main_stack_size;
__main_heap_size = __heap_end - __heap_begin;

ENTRY(systemInit)

SECTIONS
{
	.vectorTable : ALIGN(4)
	{
		*(vectors)
	} >ROM
	
	.text : ALIGN(4)
	{
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4);
		__data_i_begin = .;
	} >ROM
	
	.data : ALIGN(4)
	{
		__data_begin = .;
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		__data_end = .;
	} >RAM AT>ROM
	
	.bss : ALIGN(4)
	{
		__bss_begin = .;
		*(.bss)
		*(.bss.*)
		. = ALIGN(4);
		__bss_end = .;
	} >RAM
	
	.heap : ALIGN(4)
	{
		__init_end = .;
	} >RAM
}