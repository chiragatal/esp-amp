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
    if (xPortInIsrContext()) {
        portENTER_CRITICAL_ISR(&lock);
    } else {
        portENTER_CRITICAL(&lock);
    }
}

void esp_amp_env_exit_critical()
{
    if (xPortInIsrContext()) {
        portEXIT_CRITICAL_ISR(&lock);
    } else {
        portEXIT_CRITICAL(&lock);
    }
}

int esp_amp_env_in_isr(void)
{
    return xPortInIsrContext();
}
