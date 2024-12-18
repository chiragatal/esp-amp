/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_rom_sys.h"
#include "esp_amp_arch.h"
#include "esp_amp_platform.h"

void esp_amp_platform_delay_us(uint32_t time)
{
    esp_rom_delay_us(time);
    return;
}

void esp_amp_platform_delay_ms(uint32_t time)
{
    esp_rom_delay_us(time * 1000);
    return;
}

uint32_t esp_amp_platform_get_time_ms(void)
{
    return esp_amp_arch_get_cpu_cycle() / (esp_rom_get_cpu_ticks_per_us() * 1000);
}

void esp_amp_platform_intr_enable(void)
{
    esp_amp_arch_intr_enable();
}

void esp_amp_platform_intr_disable(void)
{
    esp_amp_arch_intr_disable();
}
