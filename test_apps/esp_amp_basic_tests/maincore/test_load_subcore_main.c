#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"

extern const uint8_t subcore_load_sub_test_bin_start[] asm("_binary_subcore_test_load_bin_start");
extern const uint8_t subcore_load_sub_test_bin_end[]   asm("_binary_subcore_test_load_bin_end");

static void subcore_basic_test_subcore_init(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_load_sub(subcore_load_sub_test_bin_start));

    /* Run subcore */
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_start_subcore());

    printf("sub core loaded with firmware and running successfully\n");
}

TEST_CASE("load sub-core image from embedded main binary", "[esp_amp]")
{
    subcore_basic_test_subcore_init();
    printf("Congratulations! The boot up is successful!\n");
}
