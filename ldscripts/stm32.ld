MEMORY
{
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
	ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
}
__stack = ORIGIN(RAM) + LENGTH(RAM);

ENTRY(main)

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
		__data_i_begin = .;
	} >ROM
	
	.data : ALIGN(4)
	{
		__data_begin = .;
		*(.data)
		*(.data.*)
		__data_end = .;
	} >RAM AT>ROM
	
	.bss : ALIGN(4)
	{
		__bss_begin = .;
		*(.bss)
		*(.bss.*)
		__bss_end = .;
	} >RAM
	
	.heap : ALIGN(4)
	{
		__heap_begin = .;
	} >RAM
}