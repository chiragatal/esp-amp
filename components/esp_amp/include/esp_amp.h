/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "esp_amp_sw_intr.h"
#include "esp_amp_sys_info.h"
#include "esp_amp_event.h"
#include "esp_amp_queue.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_rpc.h"

#include "esp_amp_env.h"
#include "esp_amp_platform.h"

#if IS_MAIN_CORE
#include "esp_amp_system.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize esp amp
 *
 * @retval 0 Init esp amp successfully
 * @retval -1 Failed to init esp amp
 */
int esp_amp_init(void);

#ifdef __cplusplus
}
#endif
