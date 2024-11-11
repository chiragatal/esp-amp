/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "esp_amp_sw_intr.h"
#include "esp_amp_rpc.h"
#include "esp_amp_sys_info.h"
#include "esp_amp_event.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_platform.h"

#if IS_MAIN_CORE
#include "esp_amp_loader.h"
#include "esp_err.h"
#endif /* IS_MAIN_CORE */

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

#if IS_MAIN_CORE
/**
 * Start subcore
 *
 * @retval 0 start subcore successfully
 * @retval -1 Failed to start subcore
 */
int esp_amp_start_subcore(void);

/**
 * Stop subcore
 */
void esp_amp_stop_subcore(void);
#endif

#ifdef __cplusplus
}
#endif
