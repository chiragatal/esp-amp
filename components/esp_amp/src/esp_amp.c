/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp.h"
#include "esp_amp_platform.h"

int esp_amp_init(void)
{
    /* init */
    esp_amp_sys_info_init();

    /* then init software interrupt */
    assert(esp_amp_sw_intr_init() == 0);

    /* then init event for synchronization */
    assert(esp_amp_event_init() == 0);

    return 0;
}

#if IS_MAIN_CORE
int esp_amp_start_subcore(void)
{
    return esp_amp_platform_start_subcore();
}

void esp_amp_stop_subcore(void)
{
    esp_amp_platform_stop_subcore();
}
#endif /* IS_MAIN_CORE */
