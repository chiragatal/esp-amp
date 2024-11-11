#include "sdkconfig.h"
#include "ld.common"
#include "esp_amp_mem_priv.h"

MEMORY
{
    ram (rwx): ORIGIN = SUBCORE_USE_HP_MEM_BOUNDARY - SUBCORE_USE_HP_MEM_SIZE , LENGTH = SUBCORE_USE_HP_MEM_SIZE
}

ENTRY(_start)

SECTIONS
{
  . = ORIGIN(ram);
  .text.entry ORIGIN(ram):
  {
    . = ALIGN(0x40);
    ASSERT(ABSOLUTE(.) % 0x40 == 0, "vector address must be 64 byte aligned");
    KEEP(*(.exception_vectors_table.text));
    KEEP(*(.exception_vectors.text));
    KEEP(*(.text.entry))
  } > ram

  .text :
  {
    *(.text .stub .text.* .gnu.linkonce.t.*)
    *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
    *(.gnu.warning)
  }
  . = ALIGN(4);

  .rodata :
  {
    *(.rodata .rodata1 .rodata.* .srodata .srodata.* .sdata2 .sdata2.* .gnu.linkonce.r.*)
    *(.rela.data .rela.data.* .rela.gnu.linkonce.r.*)
  } > ram

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > ram

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > ram

  .ctors :
  {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } > ram

  .dtors :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } > ram

  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  .data :
  {
    _data_start = .;
    __DATA_BEGIN__ = .;
    *(.data .data.* .gnu.linkonce.d.*)
    *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*)
    SORT(CONSTRUCTORS)
  } > ram
  .data1 :
  {
    *(.data1)
  } > ram
  .sdata :
  {
    __SDATA_BEGIN__ = .;
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  } > ram
  . = ALIGN(4);
  _edata = .; PROVIDE (edata = .);
  _data_size = _edata - _data_start;

  __bss_start = .;
  .sbss           :
  {
    *(.dynsbss)
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
  } > ram
  .bss            :
  {
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
  } > ram
  . = ALIGN(4);
  __BSS_END__ = .;
  __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800,
                          MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));
  _end = .; PROVIDE (end = .);

  /* Stack */
  .stack :
  {
    __stack_bottom = .;
    __stack_size_min = CONFIG_ESP_AMP_SUBCORE_STACK_SIZE_MIN;
    __stack_top = __stack_bottom + __stack_size_min;
    ASSERT(__stack_bottom + __stack_size_min == __stack_top, "Error: no space for stack");
  }

  . = __stack_top;

  .hpram.end_marker ALIGN(4) :
  {
      LONG(0x050a050a)
      LONG(0x050a050a)
  } > ram



  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* DWARF 3 */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* DWARF Extension.  */
  .debug_macro    0 : { *(.debug_macro) }
  .debug_addr     0 : { *(.debug_addr) }
  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
