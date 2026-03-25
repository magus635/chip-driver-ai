/* config/STM32F407VGT6.ld — STM32F407VGT6 链接脚本模板
 * Flash: 1024K @ 0x08000000
 * RAM:   128K  @ 0x20000000
 * CCM:   64K   @ 0x10000000 (仅 CPU 可访问，不可用于 DMA)
 *
 * 根据实际芯片修改 MEMORY 块中的大小
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size  = 0x400;  /* 1KB 堆 */
_Min_Stack_Size = 0x800;  /* 2KB 栈 */

SECTIONS
{
    /* ── 中断向量表（Flash 起始）─────────────────────── */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* ── 代码段 ─────────────────────────────────────── */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* ── 只读数据 ────────────────────────────────────── */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ── ARM 异常表 ─────────────────────────────────── */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM        : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >FLASH
    .preinit_array : { KEEP(*(.preinit_array*)) } >FLASH
    .init_array    : { KEEP(*(.init_array*)) }    >FLASH
    .fini_array    : { KEEP(*(.fini_array*)) }    >FLASH
    _sidata = LOADADDR(.data);

    /* ── CCM RAM（快速访问，不可 DMA）─────────────────── */
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } >CCMRAM AT> FLASH

    /* ── 初始化数据段 ───────────────────────────────── */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* ── 未初始化数据段 ─────────────────────────────── */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* ── 堆和栈空间检查 ─────────────────────────────── */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* ── 丢弃不需要的段 ─────────────────────────────── */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
