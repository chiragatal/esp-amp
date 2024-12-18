/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_amp.h"

#include "event.h"
#include "sys_info.h"

#define TAG "app_main"

static IRAM_ATTR int vq_recv_isr(void* args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    SemaphoreHandle_t semaphore = (SemaphoreHandle_t)args;
    xSemaphoreGiveFromISR(semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return 0;
}

typedef struct vq_recv_tsk_arg {
    SemaphoreHandle_t semaphore;
    esp_amp_queue_t* virt_queue;
} vq_recv_tsk_arg;

void recv_task(void* args)
{
    vq_recv_tsk_arg* recv_task_arg = (vq_recv_tsk_arg*)(args);
    SemaphoreHandle_t semaphore = recv_task_arg->semaphore;
    esp_amp_queue_t* virt_queue = recv_task_arg->virt_queue;

    for (;;) {
        xSemaphoreTake(semaphore, portMAX_DELAY);
        char* msg = NULL;
        uint16_t msg_size = 0;
        ESP_ERROR_CHECK(esp_amp_queue_recv_try(virt_queue, (void**)(&msg), &msg_size));
        printf("Received Msg of size %u from Sub-core: %s", msg_size, msg);
        ESP_ERROR_CHECK(esp_amp_queue_free_try(virt_queue, (void*)(msg)));
    }
}

void app_main(void)
{
    esp_amp_init();

    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(16, 0);
    int queue_len = 16;
    int queue_item_size = 64;
    esp_amp_queue_t* vq = (esp_amp_queue_t*)(malloc(sizeof(esp_amp_queue_t)));
    /* Initialize virtqueue data structure and shared memory */
    assert(esp_amp_queue_main_init(vq, queue_len, queue_item_size, vq_recv_isr, (void*)(semaphore), false, SYS_INFO_ID_VQUEUE_EXAMPLE) == 0);
    /* Bind and enable software interrupt for virtqueue */
    esp_amp_queue_intr_enable(vq);

    esp_amp_sys_info_dump();
    esp_amp_sw_intr_handler_dump();

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);
    ESP_LOGI(TAG, "Sub core linked up");

    vq_recv_tsk_arg *task_args = (vq_recv_tsk_arg*)(malloc(sizeof(vq_recv_tsk_arg)));
    assert(task_args != NULL);
    task_args->semaphore = semaphore;
    task_args->virt_queue = vq;
    xTaskCreate(recv_task, "recv_tsk", 2048, (void*)(task_args), tskIDLE_PRIORITY, NULL);

    printf("Main core started!\n");
}
