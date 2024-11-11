/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "sdkconfig.h"
#include "stdbool.h"

#ifdef __cplusplus
#include <atomic>
using std::atomic_bool;
using std::atomic_int;
using std::atomic_uint;
#else
#include <stdatomic.h>
#endif

#include "riscv/rv_utils.h"
#include "esp_amp_sw_intr.h"

#if IS_MAIN_CORE
#define TAG "main_sw_intr"
#else
#define TAG "sub_sw_intr"
#endif

#define ESP_AMP_SW_INTR_HANDLER_TABLE_LEN CONFIG_ESP_AMP_SW_INTR_HANDLER_TABLE_LEN

typedef struct {
    esp_amp_sw_intr_id_t intr_id;
    esp_amp_sw_intr_handler_t handler;
    void *arg;
} sw_intr_handler_tbl_t;

typedef struct {
    atomic_int main_core_sw_intr_st;
    atomic_int sub_core_sw_intr_st;
} esp_amp_sw_intr_st_t;
