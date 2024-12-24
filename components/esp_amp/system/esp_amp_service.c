/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if IS_MAIN_CORE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#include "esp_amp_sys_info.h"
#include "esp_amp_queue.h"
#include "esp_amp_sw_intr.h"

#include "esp_amp_env.h"
#include "esp_amp_system.h"
#include "esp_amp_service.h"

static bool s_system_service_ready = false;

#define SERVICE_QUEUE_LEN 16
#define SERVICE_QUEUE_ITEM_SIZE 128
#define SERVICE_DAEMON_STACK_SIZE 2048
#define SERVICE_DAEMON_PRIORITY 1

typedef struct {
    uint16_t srv_id;
    uint16_t param_len;
    uint8_t param[0];
} srv_pkt_hdr_t;

static esp_amp_queue_t service_queue;

#if IS_MAIN_CORE
static StaticTask_t daemon_task_stg;
static StackType_t daemon_task_stack[SERVICE_DAEMON_STACK_SIZE];
static TaskHandle_t supplicant_daemon = NULL;

static inline void handle_subcore_panic(void)
{
    if (esp_amp_subcore_panic() == 1) {
        extern void esp_amp_subcore_panic_handler(void);
        esp_amp_subcore_panic_handler();
        vTaskDelay(portMAX_DELAY); // TODO: deleting this task may cause race condition
    }
}

static void supplicant_task(void *args)
{
    (void)args;
    uint16_t srv_id;
    void *param;
    uint16_t param_len;

    while (1) {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        /* first check subcore panic */
        handle_subcore_panic();

        /* then check if there is any pending remote service */
        while (esp_amp_system_service_recv_request(&srv_id, &param, &param_len) == 0) {
            switch (srv_id) {
#if CONFIG_ESP_AMP_ROUTE_SUBCORE_PRINT
            case SYSTEM_SERVICE_ID_PRINT:
                extern void esp_amp_system_service_print(void *param, uint16_t param_len);
                esp_amp_system_service_print(param, param_len);
                break;
#endif
            default:
                break;
            }
            esp_amp_system_service_destroy_request(param);

            /* always check subcore panic to mitigate priority inversion */
            handle_subcore_panic();
        }
    }
}

int esp_amp_system_service_recv_request(uint16_t *id, void** param, uint16_t *param_len)
{
    srv_pkt_hdr_t *pkt;
    uint16_t pkt_len;

    esp_amp_env_enter_critical();
    int ret = esp_amp_queue_recv_try(&service_queue, (void **)&pkt, &pkt_len);
    esp_amp_env_exit_critical();
    if (ret != 0) {
        return -1;
    }

    *id = pkt->srv_id;
    *param = pkt->param;
    *param_len = pkt->param_len;
    return 0;
}

int esp_amp_system_service_destroy_request(void* buf)
{
    uint8_t *pkt = (uint8_t *)buf - offsetof(srv_pkt_hdr_t, param);

    esp_amp_env_enter_critical();
    int ret = esp_amp_queue_free_try(&service_queue, (void *)pkt);
    esp_amp_env_exit_critical();
    return ret;
}

void *esp_amp_system_get_supplicant(void)
{
    return (void *)supplicant_daemon;
}

#else /* IS_MAIN_CORE */

int esp_amp_system_service_create_request(void **buf, uint16_t *max_len)
{
    *buf = NULL;
    *max_len = 0;
    void *__buf;

    esp_amp_env_enter_critical();
    int ret = esp_amp_queue_alloc_try(&service_queue, &__buf, SERVICE_QUEUE_ITEM_SIZE);
    esp_amp_env_exit_critical();
    if (ret != 0) {
        return -1;
    }

    srv_pkt_hdr_t *pkt = (srv_pkt_hdr_t *)__buf;
    pkt->srv_id = 0xffff;
    pkt->param_len = 0;
    *buf = pkt->param;
    *max_len = SERVICE_QUEUE_ITEM_SIZE - sizeof(srv_pkt_hdr_t);
    return 0;
}

int esp_amp_system_service_send_request(uint16_t id, void* param, uint16_t param_len)
{
    srv_pkt_hdr_t *pkt = (srv_pkt_hdr_t *)((uint8_t *)param - offsetof(srv_pkt_hdr_t, param));
    pkt->srv_id = id;
    pkt->param_len = param_len;

    esp_amp_env_enter_critical();
    int ret = esp_amp_queue_send_try(&service_queue, (void *)pkt, SERVICE_QUEUE_ITEM_SIZE);
    esp_amp_env_exit_critical();
    if (ret != 0) {
        return -1;
    }
    return 0;
}
#endif /* IS_MAIN_CORE */

#if IS_MAIN_CORE
static IRAM_ATTR int recv_cb(void *args)
{
    (void)args;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (supplicant_daemon != NULL) {
        xTaskNotifyFromISR(supplicant_daemon, 0, eNoAction, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    return 0;
}
#else
static int notify_cb(void* args)
{
    esp_amp_sw_intr_trigger(SW_INTR_RESERVED_ID_VQUEUE);
    return 0;
}
#endif

int esp_amp_system_service_init(void)
{
#if IS_MAIN_CORE
    assert(esp_amp_queue_main_init(&service_queue, SERVICE_QUEUE_LEN, SERVICE_QUEUE_ITEM_SIZE, recv_cb, NULL, false, SYS_INFO_RESERVED_ID_SYSTEM) == 0);
    assert(esp_amp_queue_intr_enable(&service_queue) == 0);

    supplicant_daemon = xTaskCreateStatic(supplicant_task, "amp_supp",
                                          SERVICE_DAEMON_STACK_SIZE,
                                          NULL, SERVICE_DAEMON_PRIORITY,
                                          daemon_task_stack, &daemon_task_stg);
    assert(supplicant_daemon != NULL);
#else
    assert(esp_amp_queue_sub_init(&service_queue, notify_cb, NULL, true, SYS_INFO_RESERVED_ID_SYSTEM) == 0);
#endif

    s_system_service_ready = true;
    return 0;
}

bool esp_amp_system_service_is_ready(void)
{
    return s_system_service_ready;
}
