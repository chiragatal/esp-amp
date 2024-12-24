/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "ld.common"
#include "esp_amp_mem_priv.h"

MEMORY
{
  ram (rwx): ORIGIN = SUBCORE_USE_HP_MEM_START, LENGTH = SUBCORE_USE_HP_MEM_SIZE
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

  .iram1.0 ALIGN(4):
  {
      *(.iram1.*)
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
    *(.dram1 .dram1.*)
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
  . = ALIGN(8);
  __BSS_END__ = .;
  __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800,
                          MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));
  _end = .; PROVIDE (end = .);

#if CONFIG_ESP_AMP_SUBCORE_ENABLE_HEAP
  __heap_start = .;
  . += CONFIG_ESP_AMP_SUBCORE_HEAP_SIZE;
  __heap_end = ALIGN(8);
#endif

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

#include "elf_misc.ld.in"
}
