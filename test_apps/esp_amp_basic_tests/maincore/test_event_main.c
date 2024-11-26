#include "sdkconfig.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_amp_platform.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"

#define TAG "app_main"

typedef enum {
    ESP_AMP_EVENT_LOWER_1 = BIT0,
    ESP_AMP_EVENT_LOWER_2 = BIT1,
    ESP_AMP_EVENT_LOWER_3 = BIT2,
    ESP_AMP_EVENT_LOWER_4 = BIT3,
} test_lower_event_t;

typedef enum {
    ESP_AMP_EVENT_UPPER_1 = BIT4,
    ESP_AMP_EVENT_UPPER_2 = BIT5,
    ESP_AMP_EVENT_UPPER_3 = BIT6,
    ESP_AMP_EVENT_UPPER_4 = BIT7,
} test_upper_event_t;

static int test_lower_event[] = {
    ESP_AMP_EVENT_LOWER_1,
    ESP_AMP_EVENT_LOWER_2,
    ESP_AMP_EVENT_LOWER_3,
    ESP_AMP_EVENT_LOWER_4,
};

static int test_upper_event[] = {
    ESP_AMP_EVENT_UPPER_1,
    ESP_AMP_EVENT_UPPER_2,
    ESP_AMP_EVENT_UPPER_3,
    ESP_AMP_EVENT_UPPER_4,
};

static int test_event[] = {
    (ESP_AMP_EVENT_LOWER_1 | ESP_AMP_EVENT_UPPER_1),
    (ESP_AMP_EVENT_LOWER_2 | ESP_AMP_EVENT_UPPER_2),
    (ESP_AMP_EVENT_LOWER_3 | ESP_AMP_EVENT_UPPER_3),
    (ESP_AMP_EVENT_LOWER_4 | ESP_AMP_EVENT_UPPER_4),
};

/* ESP-AMP event */
#define SYS_INFO_ID_EVENT_TEST_MAIN 0x0000
#define SYS_INFO_ID_EVENT_TEST_SUB 0x0001
#define EVENT_SUBCORE_READY  (BIT0 | BIT1 | BIT2 | BIT3)
#define EVENT_MAINCORE_READY (BIT0 | BIT1 | BIT2 | BIT3)

static uint32_t early_event_st;
static uint32_t early_event_bits = BIT0 | BIT1 | BIT2 | BIT3;

static uint32_t cross_core_event_st;

extern const uint8_t subcore_event_test_bin_start[] asm("_binary_subcore_test_event_bin_start");
extern const uint8_t subcore_event_test_bin_end[]   asm("_binary_subcore_test_event_bin_end");


TEST_CASE("esp-amp event bind error when event handle is NULL", "[esp_amp]")
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);
    TEST_ASSERT_EQUAL(0, esp_amp_event_create(0x0));
    TEST_ASSERT_EQUAL(-1, esp_amp_event_bind_handle(0x0, NULL));
}

TEST_CASE("esp-amp event bind error when out of bound of event table", "[esp_amp]")
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);
    esp_amp_event_table_dump();
    EventGroupHandle_t event_handles[CONFIG_ESP_AMP_EVENT_TABLE_LEN];

    /* create events */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        event_handles[i] = xEventGroupCreate();
        TEST_ASSERT_NOT_EQUAL(NULL, event_handles[i]);
        TEST_ASSERT_EQUAL(0, esp_amp_event_create(i));
    }

    /* bind events */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        TEST_ASSERT_EQUAL(0, esp_amp_event_bind_handle(i, event_handles[i]));
        esp_amp_event_table_dump();
    }

    /* bind one more event */
    EventGroupHandle_t event_handle = xEventGroupCreate();
    TEST_ASSERT_NOT_EQUAL(NULL, event_handle);
    TEST_ASSERT_EQUAL(0, esp_amp_event_create(CONFIG_ESP_AMP_EVENT_TABLE_LEN));
    TEST_ASSERT_EQUAL(-1, esp_amp_event_bind_handle(CONFIG_ESP_AMP_EVENT_TABLE_LEN, event_handle));
    esp_amp_event_table_dump();

    /* delete events */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        vEventGroupDelete(event_handles[i]);
    }
    vEventGroupDelete(event_handle);
}

TEST_CASE("esp-amp event bind unbind", "[esp_amp]")
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create event */
    EventGroupHandle_t event_handles[CONFIG_ESP_AMP_EVENT_TABLE_LEN];
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        event_handles[i] = xEventGroupCreate();
        TEST_ASSERT_NOT_EQUAL(NULL, event_handles[i]);
        TEST_ASSERT_EQUAL(0, esp_amp_event_create(i));
    }

    /* bind event */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        TEST_ASSERT_EQUAL(0, esp_amp_event_bind_handle(i, event_handles[i]));
        esp_amp_event_table_dump();
    }

    /* unbind event */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        esp_amp_event_unbind_handle(i);
        esp_amp_event_table_dump();
    }

    /* rebind event*/
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        TEST_ASSERT_EQUAL(0, esp_amp_event_bind_handle(i, event_handles[i]));
        esp_amp_event_table_dump();
    }

    /* unbind event */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        esp_amp_event_unbind_handle(i);
        esp_amp_event_table_dump();
    }

    /* delete events */
    for (int i = 0; i < CONFIG_ESP_AMP_EVENT_TABLE_LEN; i++) {
        vEventGroupDelete(event_handles[i]);
    }
}

static void task_test_early_event(void *arg)
{
    EventGroupHandle_t test_event_group = (EventGroupHandle_t)arg;
    EventBits_t test_event_bit = xEventGroupWaitBits(test_event_group, early_event_bits, true, true, pdMS_TO_TICKS(5000));
    if ((test_event_bit & early_event_bits) == early_event_bits) {
        ESP_LOGI(TAG, "task recv event");
        early_event_st |= early_event_bits;
    } else {
        ESP_LOGI(TAG, "task timeout");
    }

    vTaskDelete(NULL);
}

TEST_CASE("esp-amp event early event not missing", "[esp_amp]")
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create event */
    TEST_ASSERT_EQUAL(0, esp_amp_event_create(0x0));
    EventGroupHandle_t test_event_group = xEventGroupCreate();
    TEST_ASSERT_NOT_NULL(test_event_group);

    /* create task */
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(task_test_early_event, "early_evt", 2048, (void *)test_event_group, 1, NULL));
    vTaskDelay(pdMS_TO_TICKS(10)); /* let task run first */

    /* set event */
    TEST_ASSERT_EQUAL(0, esp_amp_event_notify_by_id(0x0, BIT0));
    TEST_ASSERT_EQUAL(BIT0, esp_amp_event_notify_by_id(0x0, BIT1));
    TEST_ASSERT_EQUAL((BIT0 | BIT1), esp_amp_event_notify_by_id(0x0, BIT2));
    TEST_ASSERT_EQUAL((BIT0 | BIT1 | BIT2), esp_amp_event_notify_by_id(0x0, BIT3));

    /* bind event */
    TEST_ASSERT_EQUAL(0, esp_amp_event_bind_handle(0x0, test_event_group));

    vTaskDelay(pdMS_TO_TICKS(1000));
    TEST_ASSERT_EQUAL((BIT0 | BIT1 | BIT2 | BIT3), early_event_st);
}

static void task_wait_for_all(void *arg)
{
    int id = (int)arg;
    uint32_t event_bits = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_SUB, test_event[id], true, true, 10000);
    if ((event_bits & test_event[id]) == test_event[id]) {
        ESP_LOGI(TAG, "task %d recv event", id);
        cross_core_event_st |= test_event[id];
    } else {
        ESP_LOGI(TAG, "task %d timeout with event bits %p", id, (void *)event_bits);
    }
    vTaskDelete(NULL);
}

static void task_wait_for_any_lower(void *arg)
{
    int id = (int)arg;
    uint32_t event_bits = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_SUB, test_lower_event[id], true, false, 10000);
    if ((event_bits & test_lower_event[id]) != 0) {
        ESP_LOGI(TAG, "task lower %d recv event", id);
        cross_core_event_st |= (test_lower_event[id] << 8);
    } else {
        ESP_LOGI(TAG, "task lower %d timeout with event bits %p", id, (void *)event_bits);
    }
    vTaskDelete(NULL);
}

static void task_wait_for_any_upper(void *arg)
{
    int id = (int)arg;
    uint32_t event_bits = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_SUB, test_upper_event[id], true, false, 10000);
    if ((event_bits & test_upper_event[id]) != 0) {
        ESP_LOGI(TAG, "task upper %d recv event", id);
        cross_core_event_st |= (test_upper_event[id] << 8);
    } else {
        ESP_LOGI(TAG, "task upper %d timeout with event bits %p", id, (void *)event_bits);
    }
    vTaskDelete(NULL);
}

TEST_CASE("maincore & subcore can trigger event to each other", "[esp_amp]")
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create event for testing */
    TEST_ASSERT_EQUAL(0, esp_amp_event_create(SYS_INFO_ID_EVENT_TEST_MAIN));
    TEST_ASSERT_EQUAL(0, esp_amp_event_create(SYS_INFO_ID_EVENT_TEST_SUB));

    EventGroupHandle_t subcore_event = xEventGroupCreate();
    TEST_ASSERT_NOT_EQUAL(NULL, subcore_event);
    TEST_ASSERT_EQUAL(0, esp_amp_event_bind_handle(SYS_INFO_ID_EVENT_TEST_SUB, subcore_event));

    ESP_ERROR_CHECK(esp_amp_load_sub(subcore_event_test_bin_start));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* maincore notify subcore its ready state */
    esp_amp_event_notify(EVENT_MAINCORE_READY);

    /* wait for subcore ready */
    uint32_t subcore_event_bits = esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 5000);
    TEST_ASSERT_EQUAL(EVENT_SUBCORE_READY, subcore_event_bits & EVENT_SUBCORE_READY);

    char task_name[] = "taskX";
    for (int i = 0; i < 4; i++) {
        task_name[4] = '0' + i;
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(task_wait_for_all, task_name, 2048, (void *)i, tskIDLE_PRIORITY + 1, NULL));
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(task_wait_for_any_lower, task_name, 2048, (void *)i, tskIDLE_PRIORITY + 1, NULL));
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(task_wait_for_any_upper, task_name, 2048, (void *)i, tskIDLE_PRIORITY + 1, NULL));

        /* notify event */
        esp_amp_event_notify_by_id(SYS_INFO_ID_EVENT_TEST_MAIN, test_event[i]);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    TEST_ASSERT_EQUAL(0x0000ffff, cross_core_event_st);
}
