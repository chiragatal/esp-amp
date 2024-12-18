/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "esp_amp_queue.h"
#include "esp_amp_sw_intr.h"
#include "esp_amp_sys_info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_AMP_RPMSG_DATA_DEFAULT      (uint16_t)(0x0)

#define ESP_AMP_RPMSG_RESERVED_EPT_SYS_PRT      (uint16_t)(UINT16_MAX)

typedef struct esp_amp_rpmsg_head_t {
    uint16_t src_addr;                  /* source endpoint address */
    uint16_t dst_addr;                  /* destination endpoint address */
    uint16_t data_len;                  /* length of rpmsg data */
    uint16_t data_flags;                /* msg_data field property flags*/
} esp_amp_rpmsg_head_t;

typedef struct esp_amp_rpmsg_t {
    esp_amp_rpmsg_head_t msg_head;      /* rpmsg head */
    uint8_t msg_data[1];                /* rpmsg data */
} esp_amp_rpmsg_t;

typedef int (*esp_amp_ept_cb_t)(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data);

typedef struct esp_amp_rpmsg_ept_t {
    esp_amp_ept_cb_t rx_cb;     /* ISR callback function */
    void* rx_cb_data;                       /* ISR callback data */
    struct esp_amp_rpmsg_ept_t* next_ept;    /* Pointer to the next endpoint*/
    uint16_t addr;                          /* endpoint address */
} esp_amp_rpmsg_ept_t;

typedef struct esp_amp_rpmsg_dev_t {
    esp_amp_queue_t* rx_queue;
    esp_amp_queue_t* tx_queue;
    esp_amp_rpmsg_ept_t* ept_list;
    esp_amp_queue_ops_t queue_ops;
} esp_amp_rpmsg_dev_t;

/* RPMsg Endpoint Management API */

/**
 *
 * Create an endpoint with specific address
 * @param rpmsg_device      rpmsg context
 * @param ept_addr          endpoint address the created endpoint will have
 * @param ept_rx_cb         endpoint callback triggered in ISR context when receiving incoming messages, set to NULL if don't need
 * @param ept_rx_cb_data    endpoint data pointer saved in endpoint data structure, passed to the callback function when invoked
 * @param ept_ctx           allocated endpoint data structure in advance
 *
 * @retval NULL         endpoint with corresponding address exist, or ept_ctx is NULL
 * @retval ept_ctx      the same pointer as `ept_ctx` passed in
 *
 * @note Create an endpoint with specific `ept_addr` and callback function(`ept_rx_cb`).
 *       `ept_rx_cb_data` will be saved in this endpoint data structure and passed to the callback function when invoked.
 *       `ept_ctx` should be statically or dynamically allocated in advance.
 * @note This API MUST NOT be called in interrupt context.
 */
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_create_endpoint(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr, esp_amp_ept_cb_t ept_rx_cb, void* ept_rx_cb_data, esp_amp_rpmsg_ept_t* ept_ctx);


/**
 * Delete an endpoint with specific address
 * @param rpmsg_device      rpmsg context
 * @param ept_addr          the address of endpoint which should be deleted
 *
 * @retval NULL             the endpoint with corresponding `ept_addr` doesn't exist
 * @retval ept_ctx          the pointer to the deleted endpoint data structure
 *
 * @note This API MUST NOT be called in interrupt context.
 */
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_delete_endpoint(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr);


/**
 * Rebind an existing endpoint to different callback function and data pointer
 * @param rpmsg_device      rpmsg context
 * @param ept_addr          the address of endpoint which should be rebound
 * @param ept_rx_cb         new endpoint callback function
 * @param ept_rx_cb_data    new endpoint data pointer, passed to the callback function when invoked
 *
 * @retval NULL             the endpoint with corresponding `ept_addr` doesn't exist
 * @retval ept_ctx          the pointer to the rebound endpoint data structure
 *
 * @note This API MUST NOT be called in interrupt context.
 */
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_rebind_endpoint(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr, esp_amp_ept_cb_t ept_rx_cb, void* ept_rx_cb_data);

/**
 * Search for an endpoint with specific address
 * @param rpmsg_device      rpmsg context
 * @param ept_addr          the address of endpoint
 *
 * @retval NULL             the endpoint with corresponding `ept_addr` doesn't exist
 * @retval ept_ctx          the pointer to the corresponding endpoint data structure
 *
 * @note This API MUST NOT be called in interrupt context.
 */
esp_amp_rpmsg_ept_t* esp_amp_rpmsg_search_endpoint(esp_amp_rpmsg_dev_t* rpmsg_device, uint16_t ept_addr);


/**
 * Poll the next available rpmsg and execute corresponding callback function if necessary
 *
 * @param rpmsg_dev         rpmsg context
 *
 * @retval -1               no more available rpmsg to process at this time
 * @retval 0                successfully polled and processed one rpmsg, maybe still available rpmsg left, should poll again
 *
 * @note Should only be called when using polling mechanism.
 */
int esp_amp_rpmsg_poll(esp_amp_rpmsg_dev_t* rpmsg_dev);


/* RPMsg send API */

/**
 * Create and return a rpmsg buffer to read/write in place and then send with no-copy
 * @param rpmsg_dev         rpmsg context
 * @param nbytes            number of maximum bytes which you want to send with rpmsg
 * @param flags             currently reserved, should always set to ESP_AMP_RPMSG_DATA_DEFAULT
 *
 * @retval NULL             no available buffer to use / message size is larger than the maximum settings (can use esp_amp_rpmsg_get_max_size to check)
 * @retval void* ptr        successfully get the pointer to the data buffer for read/write (should be subsequently sent with nocopy version API)
 *
 * @note Must be used along with esp_amp_rpmsg_send_nocopy().
 * @note If used incorrectly with esp_amp_rpmsg_send(), the allocated buffer will never be able to be used again
 * @note Once calling this function successfully and get the data buffer pointer, the buffer MUST be sent subsequently with nocopy version API.
 *       Otherwise, the allocated buffer will never be able to be used again
 * @note This API can be called in interrupt context.
 */
void* esp_amp_rpmsg_create_message(esp_amp_rpmsg_dev_t* rpmsg_dev, uint32_t nbytes, uint16_t flags);

/**
 * Send the data buffer(rpmsg) allocated with `esp_amp_rpmsg_create_message()` to the other side without copy
 *
 * @param rpmsg_dev         rpmsg context
 * @param ept               pointer to endpoint context, indicating the identity of sender
 * @param dst_addr          destination address of the target endpoint to send
 * @param data              pointer to the data buffer allocated by esp_amp_rpmsg_send()
 * @param data_len          the size of data to send(byte), this should be smaller or equal to the "nbytes" when allocating the data buffer
 *
 * @retval 0                successfully send the data buffer to the other side
 * @retval -1               fatal error happens internally / data_len is larger than the maximum settings (can use esp_amp_rpmsg_get_max_size to check)
 *
 * @note MUST be used along with the buffer allocated by `esp_amp_rpmsg_create_message()`. Otherwise, it will cause UNDEFINED BEHAVIOR!
 * @note This API should ALWAYS succeed and return immediately if used correctly. Any errors reported by this API indicate the fatal error of rpmsg framework
 * @note This API can be called in interrupt context.
 */
int esp_amp_rpmsg_send_nocopy(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len);

/**
 * Sending the data to the other side with copy
 *
 * @param rpmsg_dev         rpmsg context
 * @param ept               pointer to endpoint context, indicating the identity of sender
 * @param dst_addr          destination address of the target endpoint to send
 * @param data              pointer to the data to be sent, MUST NOT be allocated by esp_amp_rpmsg_send()
 * @param data_len          the size of data to send(byte), this should be smaller than the maximum settings (can use esp_amp_rpmsg_get_max_size to check)
 *
 * @retval 0                successfully copy and send the data
 * @retval -1               data_len exceeds the maximum settings (can use esp_amp_rpmsg_get_max_size() to check), or there is no available buffer for use at present (should retry later)

 * @note This API will internally allocate the rpmsg data buffer, copy the data from user-provided pointer to the rpmsg data buffer, and then send it.
 * @note MUST be used standalone and without invoking `esp_amp_rpmsg_create_message()`. Otherwise, the buffer allocated by `esp_amp_rpmsg_create_message()` will never be able to be used again
 * @note This API can be used in interrupt context
 */
int esp_amp_rpmsg_send(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_rpmsg_ept_t* ept, uint16_t dst_addr, void* data, uint16_t data_len);

/**
 * Get the maximum settings of data size which one rpmsg can send at most
 * @param rpmsg_dev         rpmsg context
 *
 * @retval size             maximum data size one rpmsg can send
 *
 * @note This API can be used in interrupt context
 */
uint16_t esp_amp_rpmsg_get_max_size(esp_amp_rpmsg_dev_t* rpmsg_dev);

/**
 * Destroy(free) the rpmsg buffer after use
 *
 * @param rpmsg_dev         rpmsg context
 * @param msg_data          pointer to the allocated rpmsg buffer
 *
 * @retval 0                successfully destroy(free) the allocated rpmsg buffer
 * @retval -1               fatal error happens internally
 *
 * @note MUST be called by receiver after using the rpmsg data buffer. Otherwise, the buffer will never be able to be used again
 * @note Invoking the API on sender side or destroying rpmsg data more than once leads to UNDEFINED BEHAVIOR
 * @note This API should ALWAYS succeed and return immediately if used correctly. Any errors reported by this API indicate the fatal error of rpmsg framework
 * @note This API can be used in interrupt context
 */
int esp_amp_rpmsg_destroy(esp_amp_rpmsg_dev_t* rpmsg_dev, void* msg_data);

/**
 * Initialize the rpmsg framework on main-core
 * @param rpmsg_dev         rpmsg context, should be allocated in advance, either statically or dynamically
 * @param queue_len         the length of `Virtqueue` (number of entries, decide how many rpmsg can be sending/using simultaneously)
 * @param queue_item_size   the maximum size of each `Virtqueue` element (the maximum size of one rpmsg(including header))
 * @param notify            whether to notify the other side after sending the data (send software interrupt)
 * @param poll              whether to use the polling mechanism on this specific core, if set to false, then `esp_amp_rpmsg_intr_enable()` MUST be called later
 * @param sysinfo_id        sysinfo id of shared memory allocated for rpmsg queue buffer
 *
 * @retval 0                successfully initialize the rpmsg framework
 * @retval -1               failed to initialize
 *
 * @note this api MUST be called before any other rpmsg APIs on maincore
 */
int esp_amp_rpmsg_main_init_by_id(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_queue_t rpmsg_vqueue[], uint16_t queue_len, uint16_t queue_item_size, bool notify, bool poll, esp_amp_sys_info_id_t sysinfo_id);

/**
 * Initialize the rpmsg framework on main-core
 * @param rpmsg_dev         rpmsg context, should be allocated in advance, either statically or dynamically
 * @param queue_len         the length of `Virtqueue` (number of entries, decide how many rpmsg can be sending/using simultaneously)
 * @param queue_item_size   the maximum size of each `Virtqueue` element (the maximum size of one rpmsg(including header))
 * @param notify            whether to notify the other side after sending the data (send software interrupt)
 * @param poll              whether to use the polling mechanism on this specific core, if set to false, then `esp_amp_rpmsg_intr_enable()` MUST be called later
 *
 * @retval 0                successfully initialize the rpmsg framework
 * @retval -1               failed to initialize
 *
 * @note this api MUST be called before any other rpmsg APIs on maincore
 */
int esp_amp_rpmsg_main_init(esp_amp_rpmsg_dev_t* rpmsg_dev, uint16_t queue_len, uint16_t queue_item_size, bool notify, bool poll);

/**
 * Initialize the rpmsg framework on main-core
 * @param rpmsg_dev         rpmsg context, should be allocated in advance, either statically or dynamically
 * @param queue_len         the length of `Virtqueue` (number of entries, decide how many rpmsg can be sending/using simultaneously)
 * @param queue_item_size   the maximum size of each `Virtqueue` element (the maximum size of one rpmsg(including header))
 * @param notify            whether to notify the other side after sending the data (send software interrupt)
 * @param poll              whether to use the polling mechanism on this specific core, if set to false, then `esp_amp_rpmsg_intr_enable()` MUST be called later
 * @param sysinfo_id        sysinfo id of shared memory allocated for rpmsg queue buffer
 *
 * @retval 0                successfully initialize the rpmsg framework
 * @retval -1               failed to initialize
 *
 * @note this api MUST be called before any other rpmsg APIs on subcore
 */
int esp_amp_rpmsg_sub_init_by_id(esp_amp_rpmsg_dev_t* rpmsg_dev, esp_amp_queue_t rpmsg_vqueue[], bool notify, bool poll, esp_amp_sys_info_id_t sysinfo_id);

/**
 * Initialize the rpmsg framework on sub-core
 * @param rpmsg_dev         rpmsg context, should be allocated in advance, either statically or dynamically
 * @param notify            whether to notify the other side after sending the data (send software interrupt)
 * @param poll              whether to use the polling mechanism on this specific core, if set to false, then `esp_amp_rpmsg_intr_enable()` MUST be called later
 *
 * @retval 0                successfully initialize the rpmsg framework
 * @retval -1               failed to initialize
 *
 * @note this api MUST be called before any other rpmsg APIs on subcore
 */
int esp_amp_rpmsg_sub_init(esp_amp_rpmsg_dev_t* rpmsg_dev, bool notify, bool poll);

/**
 * Enable the rpmsg framework software interrupt handler, MUST be called when poll is set to false when initializing the rpmsg framework
 * @param rpmsg_dev         rpmsg context
 *
 * @retval 0                successfully enable the rpmsg framework software interrupt handler
 * @retval -1               failed to enable the rpmsg framework software interrupt handler
 */
int esp_amp_rpmsg_intr_enable(esp_amp_rpmsg_dev_t* rpmsg_dev);

#ifdef __cplusplus
}
#endif
