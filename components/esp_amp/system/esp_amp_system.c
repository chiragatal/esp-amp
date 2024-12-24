/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "assert.h"
#include "sdkconfig.h"
#include "esp_amp_service.h"
#include "esp_amp_system.h"
#include "esp_amp_system_priv.h"

int esp_amp_system_init(void)
{
    int ret = 0;

#if CONFIG_ESP_AMP_SYSTEM_ENABLE_SUPPLICANT
    assert(esp_amp_system_service_init() == 0);
#endif

#if IS_MAIN_CORE
    ret = esp_amp_system_panic_init();
#endif
    return ret;
}
