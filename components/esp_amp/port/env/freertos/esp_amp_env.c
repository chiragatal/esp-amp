/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "esp_amp_env.h"

/* spinlock is not used in FreeRTOS unicore mode */
static int lock;

void esp_amp_env_enter_critical()
{
    portENTER_CRITICAL(&lock);
}

void esp_amp_env_exit_critical()
{
    portEXIT_CRITICAL(&lock);
}

void esp_amp_env_enter_critical_isr()
{
    portENTER_CRITICAL_ISR(&lock);
}

void esp_amp_env_exit_critical_isr()
{
    portEXIT_CRITICAL_ISR(&lock);
}
