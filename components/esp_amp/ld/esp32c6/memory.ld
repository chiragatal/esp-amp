/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "soc/soc.h"
#include "ld.common"
#include "esp_amp_mem_priv.h"

#if CONFIG_ESP_ROM_HAS_LP_ROM
/* With LP-ROM memory layout is different due to LP ROM stack/data */
#define ULP_MEM_START_ADDRESS SOC_RTC_DRAM_LOW + RESERVE_RTC_MEM
#else
#define ULP_MEM_START_ADDRESS (SOC_RTC_DRAM_LOW)
#endif

/* Ensure the end where the shared memory starts is aligned to 8 bytes
   if updating this also update the same in ulp_lp_core_memory_shared.c
 */
#define ALIGNED_COPROC_MEM ALIGN_DOWN(CONFIG_ULP_COPROC_RESERVE_MEM, 0x8)

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = ULP_MEM_START_ADDRESS , LENGTH = 0x80
    ram(RWX) :           ORIGIN = ULP_MEM_START_ADDRESS + 0x80, LENGTH = ALIGNED_COPROC_MEM - 0x80 - CONFIG_ULP_SHARED_MEM
    shared_mem_ram(RW) : ORIGIN = ULP_MEM_START_ADDRESS + ALIGNED_COPROC_MEM - CONFIG_ULP_SHARED_MEM, LENGTH = CONFIG_ULP_SHARED_MEM
#if CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM
    hpram(RWX) :         ORIGIN = SUBCORE_USE_HP_MEM_START, LENGTH = SUBCORE_USE_HP_MEM_SIZE
#endif
}

SECTIONS
{
    .vector.text :
    {
        /*exception/interrupt vectors*/
        __mtvec_base = .;
        KEEP (*(.init.vector .init.vector.*))
    } > vector_table

    . = ORIGIN(ram);

    .text.vectors ALIGN(4):
    {
        *(.text.vectors) /* Default reset vector must in rtcram */
        *(.text.handlers) /* Interrupt & panic handlers */
    } > ram

    .iram1.0 ALIGN(4):
    {
        *(.iram1.*)
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

#if CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM

    . = ORIGIN(hpram);

    .text ALIGN(4):
    {
        *(.text)
        *(.text*)
    } >hpram

    .rodata ALIGN(4):
    {
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
    } > hpram

    .data ALIGN(4):
    {
        *(.dram1 .dram1.*)
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
    } > hpram

    .bss ALIGN(4) :
    {
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
    } > hpram

    . = ALIGN(8);
    _end = .;
    PROVIDE (end = .);

#if CONFIG_ESP_AMP_SUBCORE_ENABLE_HEAP
    __heap_start = .;
    . += CONFIG_ESP_AMP_SUBCORE_HEAP_SIZE;
    __heap_end = ALIGN(8);
#endif

    /* we need end_maker to tell loader where bss ends */
    .end_marker ALIGN(4) :
    {
        LONG(0x050a050a)
        LONG(0x050a050a)
    } > hpram

#else

    .text ALIGN(4):
    {
        *(.text)
        *(.text*)
    } >ram

    .rodata ALIGN(4):
    {
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
    } > ram

    .data ALIGN(4):
    {
        *(.dram1 .dram1.*)
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
    } > ram

    .bss ALIGN(4) :
    {
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
    } >ram

    . = ALIGN(8);
    _end = .;
    PROVIDE (end = .);

#if CONFIG_ESP_AMP_SUBCORE_ENABLE_HEAP
    __heap_start = .;
    . += CONFIG_ESP_AMP_SUBCORE_HEAP_SIZE;
    __heap_end = ALIGN(8);
#endif

#endif

    .stack :
    {
        __stack_bottom = .;
        __stack_size_min = CONFIG_ESP_AMP_SUBCORE_STACK_SIZE_MIN;
        __stack_top = ORIGIN(ram) + LENGTH(ram);
        ASSERT(__stack_top - __stack_bottom >= __stack_size_min, "Error: no space for stack");
    } > ram

    . = ORIGIN(shared_mem_ram);
    .shared_mem (ALIGN(4)) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram

#include "elf_misc.ld.in"
}
