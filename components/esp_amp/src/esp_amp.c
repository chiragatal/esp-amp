/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp.h"
#include "esp_amp_system.h"

int esp_amp_init(void)
{
    /* init software interrupt */
    assert(esp_amp_sw_intr_init() == 0);

    /* init sys info */
    assert(esp_amp_sys_info_init() == 0);

    /* init system */
    assert(esp_amp_system_init() == 0);

    /* init event */
    assert(esp_amp_event_init() == 0);

    return 0;
}
