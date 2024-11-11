/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "esp_attr.h"

#if !IS_ENV_BM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#endif

#include "esp_amp_rpmsg.h"
#include "esp_amp_platform.h"
#include "esp_amp_sys_info.h"
#include "esp_amp_sw_intr.h"

#if !IS_ENV_BM
static portMUX_TYPE rpmsg_mutex = portMUX_INITIALIZER_UNLOCKED;
#endif

static void __esp_amp_rpmsg_extend_ept_list(esp_amp_rpmsg_ept_t** ept_head, esp_amp_rpmsg_ept_t* new_ept)
{
    if (*ept_head == NULL) {
        *ept_head = new_ept;
        new_ept->next_ept = NULL;
    } else {
        // add new endpoint to the head of the linked list
        new_ept->next_ept = *ept_head;
        *ept_head = new_ept;
    }
}

/*
* Note:
* This function can either be invoked from ISR/BM/TASK context
* However, re-entrant safe is not guaranteed, which must be token into account and implemented in the higher-level
* user MUST NOT invoke this function directly
*/
static esp_amp_rpmsg_ept_t* IRAM_ATTR __esp_amp_rpmsg_search_ept(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr)
{
    // empty endpoint list
    if (rpmsg_device->ept_list == NULL) {
        return NULL;
    }

    // iterate over endpoint linked list to search for the given endpoint list
    for (esp_amp_rpmsg_ept_t* ept_ptr = rpmsg_device->ept_list; ept_ptr != NULL; ept_ptr = ept_ptr->next_ept) {
        if (ept_ptr->addr == ept_addr) {
            return ept_ptr;
        }
    }

    return NULL;
}

/*
* Note:
* This wrapper ensures re-entrant safe when being called from Task context.
* It MUST BE CALLED ONLY FROM <<FreeRTOS Task context>> after initialization if rpmsg interrupt is enabled
* <<< During initialization or When rpmsg interrupt is not enabled >>>, both Task/BM context can invoke this function
*/
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_search_ept(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr)
{
    esp_amp_rpmsg_ept_t* ept_ptr;

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
#endif

    ept_ptr = __esp_amp_rpmsg_search_ept(rpmsg_device, ept_addr);

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
#endif

    return ept_ptr;
}

/*
* This function MUST BE CALLED ONLY FROM <<FreeRTOS Task context>> after initialization if rpmsg interrupt is enabled
* <<< During initialization or When rpmsg interrupt is not enabled >>>, both Task/BM context can invoke this function
*/
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_create_ept(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr, esp_amp_ept_cb_t ept_rx_cb, void* ept_rx_cb_data, esp_amp_rpmsg_ept_t* ept_ctx)
{
    if (ept_ctx == NULL) {
        // invalid endpoint context
        return NULL;
    }

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
#endif

    if (__esp_amp_rpmsg_search_ept(rpmsg_device, ept_addr) != NULL) {
        // endpoint address already exist!
#if !IS_ENV_BM
        taskEXIT_CRITICAL(&rpmsg_mutex);
#else
        esp_amp_platform_intr_enable();
#endif
        return NULL;
    }

    ept_ctx->addr = ept_addr;
    ept_ctx->rx_cb = ept_rx_cb;
    ept_ctx->rx_cb_data = ept_rx_cb_data;
    __esp_amp_rpmsg_extend_ept_list(&(rpmsg_device->ept_list), ept_ctx);

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
#endif

    return ept_ctx;
}

/*
* This function MUST BE CALLED ONLY FROM <<FreeRTOS Task context>> after initialization if rpmsg interrupt is enabled
* <<< During initialization or When rpmsg interrupt is not enabled >>>, both Task/BM context can invoke this function
*/
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_del_ept(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr)
{

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
#endif

    esp_amp_rpmsg_ept_t* cur_ept = rpmsg_device->ept_list;
    esp_amp_rpmsg_ept_t* prev_ept = NULL;

    while (cur_ept != NULL) {
        if (cur_ept->addr == ept_addr) {
            break;
        }
        prev_ept = cur_ept;
        cur_ept = cur_ept->next_ept;
    }

    if (cur_ept == NULL) {
        // endpoint address not exist!
#if !IS_ENV_BM
        taskEXIT_CRITICAL(&rpmsg_mutex);
#else
        esp_amp_platform_intr_enable();
#endif
        return NULL;
    }

    if (prev_ept == NULL) {
        // delete the head endpoint
        rpmsg_device->ept_list = cur_ept->next_ept;
    } else {
        prev_ept->next_ept = cur_ept->next_ept;
    }
    cur_ept->next_ept = NULL;

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
#endif

    return cur_ept;
}

/*
* This function MUST BE CALLED ONLY FROM <<FreeRTOS Task context>> after initialization if rpmsg interrupt is enabled
* <<< During initialization or When rpmsg interrupt is not enabled >>>, both Task/BM context can invoke this function
*/
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_rebind_ept(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr, esp_amp_ept_cb_t ept_rx_cb, void* ept_rx_cb_data)
{

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
#endif

    esp_amp_rpmsg_ept_t* ept_ptr = __esp_amp_rpmsg_search_ept(rpmsg_device, ept_addr);
    if (ept_ptr == NULL) {
        // endpoint address not exist!
#if !IS_ENV_BM
        taskEXIT_CRITICAL(&rpmsg_mutex);
#else
        esp_amp_platform_intr_enable();
#endif
        return NULL;
    }

    ept_ptr->rx_cb = ept_rx_cb;
    ept_ptr->rx_cb_data = ept_rx_cb_data;

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
#endif

    return ept_ptr;
}

static int IRAM_ATTR __esp_amp_rpmsg_dispatcher(esp_amp_rpmsg_t* rpmsg, esp_amp_rpmsg_dev_t* rpmsg_dev)
{
    esp_amp_rpmsg_ept_t* ept = __esp_amp_rpmsg_search_ept(rpmsg_dev, rpmsg->msg_head.dst_addr);
    if (ept == NULL) {
        // can't find endpoint, ignore and return
        return -1;
    }

    if (ept->rx_cb == NULL) {
        // endpoint has no callback function, nothing to do
        return 0;
    }
    ept->rx_cb((void*)(rpmsg->msg_data), rpmsg->msg_head.data_len, rpmsg->msg_head.src_addr, ept->rx_cb_data);
    return 0;
}

/*
* If user plans to invoke this function directly, then rpmsg interrupt MUST NOT BE ENABLED
* Re-entrant safe is not guaranteed for this function. This should be token into account when being called directly by user.
*/
int IRAM_ATTR esp_amp_rpmsg_poll(esp_amp_rpmsg_dev_t* rpmsg_dev)
{
    esp_amp_rpmsg_t* rpmsg;
    uint16_t rpmsg_size;
    if (rpmsg_dev->queue_ops.q_rx(rpmsg_dev->rx_queue, (void**)(&rpmsg), &rpmsg_size) != 0) {
        // nothing to receive
        return -1;
    }

    return __esp_amp_rpmsg_dispatcher(rpmsg, rpmsg_dev);
}

static int IRAM_ATTR __esp_amp_rpmsg_rx_callback(void* data)
{
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*) data;
    while (esp_amp_rpmsg_poll(rpmsg_dev) == 0) {
        // receive and process all avaialble vqueue item
    }
    return 0;
}

static int IRAM_ATTR __esp_amp_rpmsg_tx_notify(void* data)
{
    esp_amp_sw_intr_trigger(SW_INTR_ID_VQUEUE_RECV);
    return 0;
}

int esp_amp_rpmsg_intr_enable(esp_amp_rpmsg_dev_t* rpmsg_dev)
{
    return esp_amp_sw_intr_add_handler(SW_INTR_ID_VQUEUE_RECV, rpmsg_dev->rx_queue->callback_fc, rpmsg_dev);
}

static void __esp_amp_rpmsg_init(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_queue_t vqueue[], esp_amp_queue_cb_t notify_cb, esp_amp_queue_cb_t itr_cb)
{
    rpmsg_dev->tx_queue = &vqueue[0];
    rpmsg_dev->rx_queue = &vqueue[1];
    rpmsg_dev->ept_list = NULL;
    rpmsg_dev->queue_ops.q_tx = esp_amp_queue_send_try;
    rpmsg_dev->queue_ops.q_tx_alloc = esp_amp_queue_alloc_try;
    rpmsg_dev->queue_ops.q_rx = esp_amp_queue_recv_try;
    rpmsg_dev->queue_ops.q_rx_free = esp_amp_queue_free_try;
    rpmsg_dev->tx_queue->notify_fc = notify_cb;
    rpmsg_dev->rx_queue->callback_fc = itr_cb;
}

#if IS_MAIN_CORE
static int __esp_amp_queue_main_init(esp_amp_queue_t queue[], uint16_t queue_len, uint16_t queue_item_size)
{
    if (!((queue_len > 1) && (((queue_len - 1) & queue_len) == 0))) {
        // queue_len must be power of 2
        return -1;
    }

    size_t queue_shm_size = sizeof(esp_amp_queue_conf_t) + sizeof(esp_amp_queue_desc_t) * queue_len;
    // alloc fixed-size buffer for TX/RX Virtqueue
    uint8_t* vq_buffer = (uint8_t*)(esp_amp_sys_info_alloc(SYS_INFO_ID_VQUEUE_BUFFER, 2 * queue_len * queue_item_size));
    if (vq_buffer == NULL) {
        // reserve memory not enough!
        return -1;
    }
    esp_amp_queue_conf_t* vq_tx_confg = esp_amp_sys_info_alloc(SYS_INFO_ID_VQUEUE_TX, queue_shm_size);
    esp_amp_queue_conf_t* vq_rx_confg = esp_amp_sys_info_alloc(SYS_INFO_ID_VQUEUE_RX, queue_shm_size);

    // initialize the queue config
    int ret = 0;
    ret |= esp_amp_queue_init_buffer(vq_tx_confg, queue_len, queue_item_size, (esp_amp_queue_desc_t*)((uint8_t*)(vq_tx_confg) + sizeof(esp_amp_queue_conf_t)), vq_buffer);
    ret |= esp_amp_queue_init_buffer(vq_rx_confg, queue_len, queue_item_size, (esp_amp_queue_desc_t*)((uint8_t*)(vq_rx_confg) + sizeof(esp_amp_queue_conf_t)), vq_buffer + queue_len * queue_item_size);
    ret |= esp_amp_queue_create(&queue[0], vq_tx_confg, NULL, NULL, NULL, true);
    ret |= esp_amp_queue_create(&queue[1], vq_rx_confg, NULL, NULL, NULL, false);
    // initialize the local queue structure
    return ret;
}
#else
static int __esp_amp_queue_sub_init(esp_amp_queue_t queue[])
{
    uint16_t queue_shm_size;
    // Note: the configuration is different from the queue_main_init, since the main TX is sub RX; main RX is sub TX;
    esp_amp_queue_conf_t* vq_tx_confg = esp_amp_sys_info_get(SYS_INFO_ID_VQUEUE_RX, &queue_shm_size);
    esp_amp_queue_conf_t* vq_rx_confg = esp_amp_sys_info_get(SYS_INFO_ID_VQUEUE_TX, &queue_shm_size);
    // initialize the local queue structure
    int ret = 0;
    ret |= esp_amp_queue_create(&queue[0], vq_tx_confg, NULL, NULL, NULL, true);
    ret |= esp_amp_queue_create(&queue[1], vq_rx_confg, NULL, NULL, NULL, false);
    return ret;
}
#endif

#if IS_MAIN_CORE
int esp_amp_rpmsg_main_init(esp_amp_rpmsg_dev_t* rpmsg_dev, uint16_t queue_len, uint16_t queue_item_size, bool notify, bool poll)
{
    static esp_amp_queue_t vqueue[2];
    if (__esp_amp_queue_main_init(vqueue, queue_len, queue_item_size) != 0) {
        return -1;
    }
    esp_amp_queue_cb_t tx_notify = notify ? __esp_amp_rpmsg_tx_notify : NULL;
    esp_amp_queue_cb_t rx_callback = poll ? NULL : __esp_amp_rpmsg_rx_callback;
    __esp_amp_rpmsg_init(rpmsg_dev, vqueue, tx_notify, rx_callback);

    return 0;
}
#else
int esp_amp_rpmsg_sub_init(esp_amp_rpmsg_dev_t* rpmsg_dev, bool notify, bool poll)
{
    static esp_amp_queue_t vqueue[2];
    if (__esp_amp_queue_sub_init(vqueue) != 0) {
        return -1;
    }
    esp_amp_queue_cb_t tx_notify = notify ? __esp_amp_rpmsg_tx_notify : NULL;
    esp_amp_queue_cb_t rx_callback = poll ? NULL : __esp_amp_rpmsg_rx_callback;
    __esp_amp_rpmsg_init(rpmsg_dev, vqueue, tx_notify, rx_callback);

    return 0;
}
#endif

/*
* Note: This function is re-entrant safe and MUST BE CALLED ONLY FROM:
* 1. FreeRTOS Task context
* 2. BM context with rpmsg interrupt disabled
* 3. BM context with rpmsg interrupt enabled but `esp_amp_rpmsg_create_msg_from_isr` will never be called
*/
void* esp_amp_rpmsg_create_msg(esp_amp_rpmsg_dev_t* rpmsg_dev, uint32_t nbytes, uint16_t flags)
{
    uint32_t rpmsg_size = nbytes + offsetof(esp_amp_rpmsg_t, msg_data);
    esp_amp_rpmsg_t* rpmsg;
    if (rpmsg_size >= (uint32_t)(1) << 16) {
        return NULL;
    }
#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_create_msg_from_isr` under BM environment
#endif

    int ret = rpmsg_dev->queue_ops.q_tx_alloc(rpmsg_dev->tx_queue, (void**)(&rpmsg), rpmsg_size);

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_create_msg_from_isr` under BM environment
#endif

    if (rpmsg == NULL || ret == -1) {
        return NULL;
    }

    rpmsg->msg_head.data_flags = flags;
    rpmsg->msg_head.data_len = nbytes;

    return (void*)((uint8_t*)(rpmsg) + offsetof(esp_amp_rpmsg_t, msg_data));
}


/*
* Note: This function MUST BE CALLED ONLY FROM
* 1. FreeRTOS ISR context
* 2. ISR context in BM environment but `esp_amp_rpmsg_create_msg` will never be called
*/
void* IRAM_ATTR esp_amp_rpmsg_create_msg_from_isr(esp_amp_rpmsg_dev_t* rpmsg_dev, uint32_t nbytes, uint16_t flags)
{
    uint32_t rpmsg_size = nbytes + offsetof(esp_amp_rpmsg_t, msg_data);
    esp_amp_rpmsg_t* rpmsg;
    if (rpmsg_size >= (uint32_t)(1) << 16) {
        return NULL;
    }

    int ret = rpmsg_dev->queue_ops.q_tx_alloc(rpmsg_dev->tx_queue, (void**)(&rpmsg), rpmsg_size);

    if (rpmsg == NULL || ret == -1) {
        return NULL;
    }

    rpmsg->msg_head.data_flags = flags;
    rpmsg->msg_head.data_len = nbytes;

    return (void*)((uint8_t*)(rpmsg) + offsetof(esp_amp_rpmsg_t, msg_data));
}

/*
* This function should be called either from:
* 1. FreeRTOS Task context
* 2. BM context with rpmsg interrupt disabled
* 3. BM context with rpmsg interrupt enabled but `esp_amp_rpmsg_send_from_isr` will never be called
* This function will return immediately without block
*/
int esp_amp_rpmsg_send(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len)
{

    if (data == NULL || data_len == 0) {
        return -1;
    }

    void* buffer = esp_amp_rpmsg_create_msg(rpmsg_dev, data_len, ESP_AMP_RPMSG_DATA_DEFAULT);

    if (buffer == NULL) {
        return -1;
    }

    for (uint16_t i = 0; i < data_len; i++) {
        ((uint8_t*)(buffer))[i] = ((uint8_t*)(data))[i];
    }

    return esp_amp_rpmsg_send_nocopy(rpmsg_dev, ept, dst_addr, buffer, data_len);
}

/*
* This function should be called either from:
* 1. ISR context in FreeRTOS environment
* 2. ISR context in BM environment when user can ensure that `esp_amp_rpmsg_send` will not be called in BM context
*/
int IRAM_ATTR esp_amp_rpmsg_send_from_isr(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len)
{
    if (data == NULL || data_len == 0) {
        return -1;
    }

    void* buffer = esp_amp_rpmsg_create_msg_from_isr(rpmsg_dev, data_len, ESP_AMP_RPMSG_DATA_DEFAULT);

    if (buffer == NULL) {
        return -1;
    }

    for (uint16_t i = 0; i < data_len; i++) {
        ((uint8_t*)(buffer))[i] = ((uint8_t*)(data))[i];
    }

    return esp_amp_rpmsg_send_nocopy_from_isr(rpmsg_dev, ept, dst_addr, buffer, data_len);
}

/*
* This function should be called either from:
* 1. FreeRTOS Task context
* 2. BM context with rpmsg interrupt disabled
* 3. BM context with rpmsg interrupt enabled but `esp_amp_rpmsg_send_nocopy_from_isr` will never be called
* This function will return immediately without block and should never fail
*/
int esp_amp_rpmsg_send_nocopy(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len)
{
    esp_amp_rpmsg_t* rpmsg = (esp_amp_rpmsg_t*)((uint8_t*)(data) - offsetof(esp_amp_rpmsg_t, msg_data));
    rpmsg->msg_head.data_len = data_len;
    rpmsg->msg_head.dst_addr = dst_addr;
    rpmsg->msg_head.src_addr = ept->addr;

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_send_nocopy_from_isr` under BM environment
#endif

    int ret = rpmsg_dev->queue_ops.q_tx(rpmsg_dev->tx_queue, rpmsg, rpmsg_dev->tx_queue->max_item_size);

#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_send_nocopy_from_isr` under BM environment
#endif

    return ret;
}

/*
* This function should be called either from:
* 1. ISR context in FreeRTOS environment
* 2. ISR context in BM environment when user can ensure that `esp_amp_rpmsg_send_nocopy` will not be called in BM context
*/
int IRAM_ATTR esp_amp_rpmsg_send_nocopy_from_isr(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len)
{
    esp_amp_rpmsg_t* rpmsg = (esp_amp_rpmsg_t*)((uint8_t*)(data) - offsetof(esp_amp_rpmsg_t, msg_data));
    rpmsg->msg_head.data_len = data_len;
    rpmsg->msg_head.dst_addr = dst_addr;
    rpmsg->msg_head.src_addr = ept->addr;

    return rpmsg_dev->queue_ops.q_tx(rpmsg_dev->tx_queue, rpmsg, rpmsg_dev->tx_queue->max_item_size);
}

/*
* This function should be called either from:
* 1. FreeRTOS Task context
* 2. BM context with rpmsg interrupt disabled
* 3. BM context with rpmsg interrupt enabled but `esp_amp_rpmsg_destroy_from_isr` will never be called
* This function will return immediately without block and should never fail
*/
int esp_amp_rpmsg_destroy(esp_amp_rpmsg_dev_t* rpmsg_dev, void* msg_data)
{
    esp_amp_rpmsg_t* rpmsg = (esp_amp_rpmsg_t*)((uint8_t*)(msg_data) - offsetof(esp_amp_rpmsg_t, msg_data));

#if !IS_ENV_BM
    taskENTER_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_disable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_destroy_from_isr` under BM environment
#endif

    int ret = rpmsg_dev->queue_ops.q_rx_free(rpmsg_dev->rx_queue, rpmsg);
#if !IS_ENV_BM
    taskEXIT_CRITICAL(&rpmsg_mutex);
#else
    esp_amp_platform_intr_enable();
    // should add support for disabling interrupt under BM environment,
    // so that this function can also be called along with `esp_amp_rpmsg_destroy_from_isr` under BM environment
#endif

    return ret;
}

/*
* This function returns the maximum size of the rpmsg payload
* Safe to call from both BM/Task/ISR context
*/
uint16_t IRAM_ATTR esp_amp_rpmsg_get_max_size(esp_amp_rpmsg_dev_t* rpmsg_dev)
{
    return (uint16_t)(rpmsg_dev->tx_queue->max_item_size - offsetof(esp_amp_rpmsg_t, msg_data));
}

/*
* This function should be called either from:
* 1. ISR context in FreeRTOS environment
* 2. ISR context in BM environment when user can ensure that `esp_amp_rpmsg_destroy` will not be called in BM context
*/
int esp_amp_rpmsg_destroy_from_isr(esp_amp_rpmsg_dev_t* rpmsg_dev, void* msg_data)
{

    esp_amp_rpmsg_t* rpmsg = (esp_amp_rpmsg_t*)((uint8_t*)(msg_data) - offsetof(esp_amp_rpmsg_t, msg_data));

    return rpmsg_dev->queue_ops.q_rx_free(rpmsg_dev->rx_queue, rpmsg);
}

