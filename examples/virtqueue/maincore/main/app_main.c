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
#include "esp_amp_queue.h"

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
    // allocate the virtqueue config in shared memory, which can be used by the subcore to create the virtqueue handler
    esp_amp_queue_conf_t* vq_conf = (esp_amp_queue_conf_t*)(esp_amp_sys_info_alloc(SYS_INFO_ID_VQUEUE_CONF, sizeof(esp_amp_queue_conf_t)));
    assert(vq_conf != NULL);
    esp_amp_sys_info_dump();
    esp_amp_queue_t* vq = (esp_amp_queue_t*)(malloc(sizeof(esp_amp_queue_t)));
    // since subcore can direcly access the main-core's memory currently, we can allocate the queue data structure on main-core's heap
    esp_amp_queue_desc_t* vq_desc = (esp_amp_queue_desc_t*)(malloc(queue_len * sizeof(esp_amp_queue_desc_t)));
    void* vq_buffer = malloc(queue_len * queue_item_size);

    // initialize queue buffer and descriptor
    esp_amp_queue_init_buffer(vq_conf, queue_len, queue_item_size, vq_desc, vq_buffer);
    // create the queue handler (as the role of `remote-core`)
    esp_amp_queue_create(vq, vq_conf, vq_recv_isr, NULL, (void*)(semaphore), false);

    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, vq->callback_fc, vq->priv_data) == 0);

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
