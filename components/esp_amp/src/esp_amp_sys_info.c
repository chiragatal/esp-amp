/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_amp_priv.h"
#include "esp_amp_sys_info.h"
#if IS_MAIN_CORE
#include "heap_memory_layout.h"
#endif
#include "esp_amp_log.h"

#define TAG "sys_info"

#define ESP_AMP_SYS_INFO_ID_MAX 0xffff

#if CONFIG_ESP_AMP_SHARED_MEM_IN_HP
#define ESP_AMP_SYS_INFO_ADDR (ESP_AMP_SHARED_MEM_BOUNDARY - CONFIG_ESP_AMP_SHARED_MEM_SIZE)
#elif ONFIG_ESP_AMP_SHARED_MEM_IN_LP
#define ESP_AMP_SYS_INFO_ADDR (RTC_SLOW_MEM + CONFIG_ULP_COPROC_RESERVE_MEM)
#endif

#if IS_MAIN_CORE
SOC_RESERVE_MEMORY_REGION(ESP_AMP_SYS_INFO_ADDR, ESP_AMP_SYS_INFO_ADDR + CONFIG_ESP_AMP_SHARED_MEM_SIZE, esp_amp_sys_info);
#endif


typedef struct sys_info_header_t {
    uint16_t info_id;
    uint16_t size;                  /* original size in byte */
    struct sys_info_header_t* next; /* offset related to buffer */
} sys_info_header_t;

#define ESP_AMP_SYS_INFO_BUFFER_SIZE (CONFIG_ESP_AMP_SHARED_MEM_SIZE)

static sys_info_header_t* const s_esp_amp_sys_info = (sys_info_header_t *)ESP_AMP_SYS_INFO_ADDR;

#if IS_MAIN_CORE
static uint16_t get_size_word(uint16_t size)
{
    uint16_t size_in_word = size >> 2;
    if (size & 0x3) {
        size_in_word += 1;
    }
    return size_in_word;
}
#endif /* IS_MAIN_CORE */

void * IRAM_ATTR esp_amp_sys_info_get(uint16_t info_id, uint16_t *size)
{
    sys_info_header_t *sys_info_entry = s_esp_amp_sys_info->next;
    while (sys_info_entry != NULL) {
        if (sys_info_entry->info_id == info_id) {
            if (size != NULL) {
                *size = sys_info_entry->size;
            }

            void* buffer = (void*)((uint8_t*)(sys_info_entry) + sizeof(sys_info_header_t));

            ESP_AMP_LOGD(TAG, "get info:%x, size:0x%x, addr:%p", sys_info_entry->info_id, sys_info_entry->size, buffer);

            return buffer;
        }

        sys_info_entry = sys_info_entry->next;
    }

    ESP_AMP_LOGE(TAG, "INFO_ID(0x%x) not found", info_id);
    return NULL;
}

#if IS_MAIN_CORE
void * IRAM_ATTR esp_amp_sys_info_alloc(uint16_t info_id, uint16_t size)
{
    sys_info_header_t *sys_info_entry = s_esp_amp_sys_info;

    while (sys_info_entry->next != NULL) {
        if (sys_info_entry->next->info_id == info_id) {
            ESP_AMP_LOGE(TAG, "Info id(%x) already exist", info_id);
            return NULL;
        }
        sys_info_entry = sys_info_entry->next;
    }

    void* next_sys_info_entry_start = (uint8_t*)(sys_info_entry) + sizeof(sys_info_header_t) + 4 * get_size_word(sys_info_entry->size);
    void* next_sys_info_entry_end = (uint8_t*)(next_sys_info_entry_start) + sizeof(sys_info_header_t) + 4 * get_size_word(size);
    void* buffer = (void*)((uint8_t*)(next_sys_info_entry_start) + sizeof(sys_info_header_t));

    if ((uint8_t*)(next_sys_info_entry_end) > (uint8_t*)(s_esp_amp_sys_info) + ESP_AMP_SYS_INFO_BUFFER_SIZE) {
        ESP_AMP_LOGE(TAG, "No space in buffer");
        return NULL;
    }

    ESP_AMP_LOGD(TAG, "alloc info:%x, size:0x%x, addr:%p", info_id, size, buffer);

    sys_info_entry->next = next_sys_info_entry_start;
    sys_info_entry->next->next = NULL;
    sys_info_entry->next->info_id = info_id;
    sys_info_entry->next->size = size;

    return buffer;
}

#endif /* IS_MAIN_CORE */

int esp_amp_sys_info_init(void)
{
#if IS_MAIN_CORE
    s_esp_amp_sys_info->info_id = ESP_AMP_SYS_INFO_ID_MAX;
    s_esp_amp_sys_info->size = 0;
    s_esp_amp_sys_info->next = NULL;
#endif /* IS_MAIN_CORE */
    return 0;
}

void esp_amp_sys_info_dump(void)
{
    ESP_AMP_LOGI(TAG, "sys_info: %p", s_esp_amp_sys_info);
    ESP_AMP_LOGI(TAG, "==================================");
    ESP_AMP_LOGI(TAG, "INFO_ID\tSIZE\tADDRESS");
    sys_info_header_t *sys_info_entry = s_esp_amp_sys_info->next;
    while (sys_info_entry != NULL) {
        ESP_AMP_LOGI(TAG, "0x%04x\t0x%04x\t%p", sys_info_entry->info_id, sys_info_entry->size, (void*)((uint8_t*)(s_esp_amp_sys_info) + sizeof(sys_info_header_t)));
        sys_info_entry = sys_info_entry->next;
    }

    /* print buffer */
    ESP_AMP_LOGI(TAG, "==================================");
    ESP_AMP_LOG_BUFFER_HEXDUMP(TAG, (void*)(s_esp_amp_sys_info), ESP_AMP_SYS_INFO_BUFFER_SIZE, ESP_AMP_LOG_DEBUG);
}