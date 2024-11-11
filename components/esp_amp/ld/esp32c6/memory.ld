/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
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

#define SUBCORE_USE_HP_MEM_SIZE CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM_SIZE

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = ULP_MEM_START_ADDRESS , LENGTH = 0x80
    ram(RWX) :           ORIGIN = ULP_MEM_START_ADDRESS + 0x80, LENGTH = CONFIG_ULP_COPROC_RESERVE_MEM - 0x80 - CONFIG_ULP_SHARED_MEM
#if CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM
    hpram(RWX) :         ORIGIN = SUBCORE_USE_HP_MEM_BOUNDARY - SUBCORE_USE_HP_MEM_SIZE , LENGTH = SUBCORE_USE_HP_MEM_SIZE
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
    } > hpram

    .data ALIGN(4):
    {
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
        PROVIDE(end = .);
    } > hpram

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
    } > ram

    .data ALIGN(4):
    {
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
        PROVIDE(end = .);
    } >ram

#endif

    .stack :
    {
        __stack_bottom = .;
        __stack_size_min = CONFIG_ESP_AMP_SUBCORE_STACK_SIZE_MIN;
        __stack_top = ORIGIN(ram) + LENGTH(ram);
        ASSERT(__stack_top - __stack_bottom >= __stack_size_min, "Error: no space for stack");
    } > ram

}
