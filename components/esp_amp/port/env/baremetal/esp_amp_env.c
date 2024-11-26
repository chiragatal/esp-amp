/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_amp_platform.h"
#include "esp_amp_env.h"

void esp_amp_env_enter_critical(void)
{
    esp_amp_platform_intr_disable();
}

void esp_amp_env_exit_critical(void)
{
    esp_amp_platform_intr_enable();
}

void esp_amp_env_enter_critical_isr(void)
{
    esp_amp_platform_intr_disable();
}

void esp_amp_env_exit_critical_isr(void)
{
    esp_amp_platform_intr_enable();
}
