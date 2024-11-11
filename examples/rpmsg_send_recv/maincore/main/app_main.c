/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <sdkconfig.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "event.h"

#define TAG "app_main"

esp_amp_rpmsg_dev_t rpmsg_dev;
esp_amp_rpmsg_ept_t rpmsg_ept[3];

void ept0_task_ctx(void* arg)
{
    QueueHandle_t msgQueue = (QueueHandle_t) arg;
    void* data;
    while (true) {
        xQueueReceive(msgQueue, &data, portMAX_DELAY);
        printf("EPT0: Message from Sub core ==> %s\n", (char*)(data));
        esp_amp_rpmsg_destroy(&rpmsg_dev, data);
    }
    vTaskDelete(NULL);
}

void ept1_task_ctx(void* arg)
{
    QueueHandle_t msgQueue = (QueueHandle_t) arg;
    void* data;
    int count = 0;
    while (true) {
        xQueueReceive(msgQueue, &data, portMAX_DELAY);
        printf("EPT1: Message from Sub core ==> %s\n", (char*)(data));
        esp_amp_rpmsg_destroy(&rpmsg_dev, data);
        data = esp_amp_rpmsg_create_msg(&rpmsg_dev, 32, ESP_AMP_RPMSG_DATA_DEFAULT);
        if (data != NULL) {
            snprintf(data, 30, "Rsp from EPT1: %d", count++);
            assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[1], 0, data, 30) == 0);
        } else {
            printf("EPT1: Failed to create new message!\n");
        }
    }
    vTaskDelete(NULL);
}

void ept2_task_ctx(void* arg)
{
    QueueHandle_t msgQueue = (QueueHandle_t) arg;
    int algo_counter = 0;
    uint16_t dst_addr = 0;
    int* result = NULL;
    for (;;) {
        int a = rand() % 1000;
        int b = rand() % 1000;
        int c = 0;
        if (algo_counter) {
            c = a + b;
            dst_addr = 1;
            printf("Generating %d + %d = %d\n", a, b, c);
        } else {
            c = a * b;
            dst_addr = 2;
            printf("Generating %d x %d = %d\n", a, b, c);
        }
        algo_counter = !algo_counter;
        int* data = (int*)(esp_amp_rpmsg_create_msg(&rpmsg_dev, sizeof(int) * 2, ESP_AMP_RPMSG_DATA_DEFAULT));
        if (data != NULL) {
            data[0] = a;
            data[1] = b;
            assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[2], dst_addr, data, sizeof(int) * 2) == 0);
            printf("Sending requests successfully. Waiting for response\n");
            xQueueReceive(msgQueue, &result, portMAX_DELAY);
            if (*result == c) {
                printf("Expected %d, got %d, PASS\n", *result, c);
            } else {
                printf("Expected %d, got %d, INVALID\n", *result, c);
            }
            esp_amp_rpmsg_destroy(&rpmsg_dev, result);
        } else {
            printf("Failed to send requests!\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

int IRAM_ATTR ept_isr_ctx(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    // zero copy
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    QueueHandle_t msgQueue = (QueueHandle_t) rx_cb_data;
    if (xQueueSendFromISR(msgQueue, &msg_data, &xHigherPriorityTaskWoken) != pdTRUE) {
        esp_amp_rpmsg_destroy_from_isr(&rpmsg_dev, msg_data);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    return 0;
}


void app_main(void)
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* init transport layer */
    int ret;

#if EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE
    ret = esp_amp_rpmsg_main_init(&rpmsg_dev, 32, 128, true, false);
#else
    ret = esp_amp_rpmsg_main_init(&rpmsg_dev, 32, 128, true, true);
#endif /* EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE */

    QueueHandle_t ept0_msg_queue = xQueueCreate(32, sizeof(void*));
    QueueHandle_t ept1_msg_queue = xQueueCreate(32, sizeof(void*));
    QueueHandle_t ept2_msg_queue = xQueueCreate(32, sizeof(void*));
    assert(ret == 0);
    assert(esp_amp_rpmsg_create_ept(&rpmsg_dev, 0, ept_isr_ctx, (void*)(ept0_msg_queue), &rpmsg_ept[0]) != NULL);
    assert(esp_amp_rpmsg_create_ept(&rpmsg_dev, 1, ept_isr_ctx, (void*)(ept1_msg_queue), &rpmsg_ept[1]) != NULL);
    assert(esp_amp_rpmsg_create_ept(&rpmsg_dev, 2, ept_isr_ctx, (void*)(ept2_msg_queue), &rpmsg_ept[2]) != NULL);

#if EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE
    esp_amp_rpmsg_intr_enable(&rpmsg_dev);
#endif /* EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE */

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    xTaskCreate(ept0_task_ctx, "ept0", 2048, (void*)ept0_msg_queue, tskIDLE_PRIORITY, NULL);
    xTaskCreate(ept1_task_ctx, "ept1", 2048, (void*)ept1_msg_queue, tskIDLE_PRIORITY, NULL);
    xTaskCreate(ept2_task_ctx, "ept2", 2048, (void*)ept2_msg_queue, tskIDLE_PRIORITY, NULL);

    printf("Main core started!\n");

#ifdef CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE
    printf("Demonstrating interrupt-based RPMsg handling on maincore\r\n");
#else
    printf("Demonstrating polling-based RPMsg handling on maincore\r\n");
#endif /* CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE */


#if !EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE
    /* poll rpmsg */
    while (1) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif /* !EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE */

}