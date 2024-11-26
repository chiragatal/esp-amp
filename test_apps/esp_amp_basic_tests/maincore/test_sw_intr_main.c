#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"

#include "unity.h"
#include "unity_test_runner.h"

extern const uint8_t subcore_sw_intr_bin_start[] asm("_binary_subcore_test_sw_intr_bin_start");
extern const uint8_t subcore_sw_intr_bin_end[]   asm("_binary_subcore_test_sw_intr_bin_end");

static uint8_t main_sw_intr_record[4];

static const DRAM_ATTR char TAG[] = "test_sw_intr";

static IRAM_ATTR int sw_intr_id0_handler(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id0_handler() called");
    main_sw_intr_record[0]++;
    return 0;
}

static IRAM_ATTR int sw_intr_id1_handler(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id1_handler() called");
    main_sw_intr_record[1]++;
    return 0;
}

static IRAM_ATTR int sw_intr_id2_handler(void *arg)
{
    ESP_DRAM_LOGI("", "sw_intr_id2_handler() called");
    main_sw_intr_record[2]++;
    return 0;
}

static IRAM_ATTR int sw_intr_id3_handler(void *arg)
{
    ESP_DRAM_LOGI("", "sw_intr_id3_handler() called");
    main_sw_intr_record[3]++;
    return 0;
}


TEST_CASE("maincore & subcore can trigger software interrupt to each other", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    TEST_ASSERT(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler, NULL) == 0);
    TEST_ASSERT(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler, NULL) == 0);
    TEST_ASSERT(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler, NULL) == 0);
    TEST_ASSERT(esp_amp_sw_intr_add_handler(SW_INTR_ID_3, sw_intr_id3_handler, NULL) == 0);

    esp_amp_sw_intr_handler_dump();

    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_load_sub(subcore_sw_intr_bin_start));
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_start_subcore());
    vTaskDelay(pdMS_TO_TICKS(1000)); /* wait for subcore to start */

    for (int i = 0; i < 16; i++) {
        printf("maincore trigger intr 0...\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_0);
        vTaskDelay(pdMS_TO_TICKS(100));

        printf("maincore trigger intr 1...\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_1);
        vTaskDelay(pdMS_TO_TICKS(100));

        printf("maincore trigger intr 2...\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_2);
        vTaskDelay(pdMS_TO_TICKS(100));

        printf("maincore trigger intr 3...\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_3);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    uint8_t main_sw_intr_expect[4] = {0x10, 0x10, 0x10, 0x10};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(main_sw_intr_expect, main_sw_intr_record, 4);
}
