/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "sdkconfig.h"
#include "stdint.h"
#include "esp_amp_rpmsg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_AMP_RPC_MAX_PENDING_REQ CONFIG_ESP_AMP_RPC_MAX_PENDING_REQ

/**
 * esp amp rpc status code
 *
 * indicate the internal status of rpc packet
 */
typedef enum {
    ESP_AMP_RPC_STATUS_OK = 0, /* 0 */
    ESP_AMP_RPC_STATUS_FAILED, /* internal error */
    ESP_AMP_RPC_STATUS_INVALID_ARG,
    ESP_AMP_RPC_STATUS_PENDING,
    ESP_AMP_RPC_STATUS_QUEUE_FULL, /* reach max pending limit */
    ESP_AMP_RPC_STATUS_NO_SERVICE, /* invalid service id */
    ESP_AMP_RPC_STATUS_EXEC_FAILED, /* execute  on server */
    ESP_AMP_RPC_STATUS_TIMEOUT, /* timer time out */
    ESP_AMP_RPC_STATUS_NO_MEM, /* memory allocation failed */
    ESP_AMP_RPC_STATUS_BAD_PACKET,
} esp_amp_rpc_status_t;

/* rpc request handle exposed to user app */
typedef void *esp_amp_rpc_req_handle_t;

/* rpc packet for both request & response */
typedef struct {
    uint16_t req_id;
    uint16_t service_id;
    uint16_t status;
    uint16_t params_len;
    uint8_t params[0];
} esp_amp_rpc_pkt_t;

/**
 * rpc service invoked by rpc server with corresponding service id
 * transport buffer for output parameters is pre-allocated by rpc server and forwarded to rpc server shim.
 * if the output parameters cannot fit in the preallocated buffer, send back the error code to client
 *
 * @param[in] params_in input parameters of rpc service
 * @param[in] params_in_len length of input parameters of rpc service
 * @param[inout] params_out output paramsters of rpc service (params_out is pre-allocated)
 * @param[inout] params_out_len length of output parameters of rpc service
 */
typedef esp_amp_rpc_status_t (* esp_amp_rpc_service_func_t)(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len);

typedef int esp_amp_rpc_service_id_t;
typedef struct {
    esp_amp_rpc_service_id_t id;
    esp_amp_rpc_service_func_t handler;
} esp_amp_rpc_service_t;

/**
 * rpc client cb
 * callback when client receives response or the request is timeout
 */
typedef void (* esp_amp_rpc_req_cb_t)(esp_amp_rpc_status_t status, void *params_out, uint16_t params_out_len);

#if !IS_ENV_BM

/**
 * Initialize rpc client
 * This client is global and can be shared by all tasks. Number of pending requests is specified by Kconfig macro
 * CONFIG_ESP_AMP_RPC_MAX_PENDING_REQ.
 *
 * @param[in] rpmsg_dev rpmsg device of current core
 * @param[in] client_addr rpc client's endpoint address
 * @param[in] server_addr rpc server's endpoint address
 * @param[in] task_priority rpc client's task priority
 * @param[in] stack_size rpc client's stack size
 *
 * @retval ESP_AMP_RPC_STATUS_OK successful to init client
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to init client
 */
esp_amp_rpc_status_t esp_amp_rpc_client_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr, int task_priority, int stack_size);

#else

/**
 * Initialize rpc client
 * This client is global and can be shared by all tasks. Number of pending requests is specified by Kconfig macro
 * CONFIG_ESP_AMP_RPC_MAX_PENDING_REQ.
 *
 * @param[in] rpmsg_dev rpmsg device of current core
 * @param[in] client_addr rpc client's endpoint address
 * @param[in] server_addr rpc server's endpoint address
 *
 * @retval ESP_AMP_RPC_STATUS_OK successful to init client
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to init client
 */
esp_amp_rpc_status_t esp_amp_rpc_client_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr);

#endif /* !IS_ENV_BM */

/**
 * Create an RPC request
 * Internally a pending request is created and added to pending list which tracks all pending requests
 * sent by RPC client. Transport buffer is also allocated to construct RPC packet
 *
 * @param[in] service_id service id of rpc requests to be executed by server
 * @param[in] params_in input parameters of the rpc request
 * @param[in] params_in_len length of the input parameters of the rpc request
 * @retval NULL failed to create the RPC request
 * @retval others handle of the RPC request
 */
esp_amp_rpc_req_handle_t esp_amp_rpc_client_create_request(uint16_t service_id, void *params_in, uint16_t params_in_len);

#if !IS_ENV_BM

/**
 * Execute the created RPC request
 *
 * @param[in] req handle of the created RPC request
 * @param[out] params_out output parameters of the RPC request
 * @param[out] params_out_len length of the parameters of the rpc request
 * @param[in] timeout_ms maximum waiting time (in millisecond) before timeout. -1 means waiting forever
 * @retval ESP_AMP_RPC_STATUS_OK successfully send out the request
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to send out the request (QUEUE_FULL)
 */
esp_amp_rpc_status_t esp_amp_rpc_client_execute_request(esp_amp_rpc_req_handle_t req, void **params_out, int *params_out_len, uint32_t timeout_ms);

#else

/**
 * Execute the created RPC request
 *
 * @param[in] req handle of the created RPC request
 * @param[in] cb callback handler for the RPC request
 * @param[in] timeout_ms maximum waiting time (in millisecond) before timeout.
 * @retval ESP_AMP_RPC_STATUS_OK successfully send out the request
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to send out the request (QUEUE_FULL)
 */
esp_amp_rpc_status_t esp_amp_rpc_client_execute_request_with_cb(esp_amp_rpc_req_handle_t req, esp_amp_rpc_req_cb_t cb, uint32_t timeout_ms);

#endif /* IS_ENV_BM */

/**
 * Destroy the RPC request
 * Pending request and transport buffer are released
 *
 * @param[in] req handle of the created RPC request
 */
void esp_amp_rpc_client_destroy_request(esp_amp_rpc_req_handle_t req);


/**
 * Start the rpc client by creating both sending tasks and receiving tasks
 * This API can only be used in freertos environment
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully start the rpc client
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to start the rpc client
 */
esp_amp_rpc_status_t esp_amp_rpc_client_run(void);


/**
 * Complete the timeout request by triggering timeout cb & remove from pending list
 * This API can only be used in baremetal environment
 */
void esp_amp_rpc_client_complete_timeout_request(void);


/**
 * Stop the rpc client
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully stopped the rpc client
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to stop the rpc client
 */
esp_amp_rpc_status_t esp_amp_rpc_client_stop(void);


/**
 * Deinitialize the rpc client
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully deinit the rpc client
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to deinit the rpc client
 */
esp_amp_rpc_status_t esp_amp_rpc_client_deinit(void);

#if !IS_ENV_BM

/**
 * Initialize an rpc server
 *
 * @param[in] rpmsg_dev rpmsg device of current core
 * @param[in] client_addr rpc client's endpoint address
 * @param[in] server_addr rpc server's endpoint address
 * @param[in] task_priority rpc client's task priority
 * @param[in] stack_size rpc client's stack size
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully initialize the rpc server
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to initialize the rpc server
 */
esp_amp_rpc_status_t esp_amp_rpc_server_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr, int task_priority, int stack_size);

#else

/**
 * Initialize an rpc server
 *
 * @param[in] rpmsg_dev rpmsg device of current core
 * @param[in] client_addr rpc client's endpoint address
 * @param[in] server_addr rpc server's endpoint address
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully initialize the rpc server
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to initialize the rpc server
 */
esp_amp_rpc_status_t esp_amp_rpc_server_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr);

#endif /* !IS_ENV_BM */

/**
 * Add a new service handler to rpc server's service table
 * This API is thread-safe. Adding new services to a running server is allowed
 *
 * @param[in] srv_id identifier for the service handler
 * @param[in] srv_func service handler
 * @retval ESP_AMP_RPC_STATUS_OK successfully add the handler to server's service table
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to add the handler to server's service table
 */
esp_amp_rpc_status_t esp_amp_rpc_server_add_service(esp_amp_rpc_service_id_t srv_id, esp_amp_rpc_service_func_t srv_func);


/**
 * Create an rpc server task and start to process incoming RPC requests
 * Call this API to a running server will have no effect
 *
 * @retval ESP_AMP_RPC_STATUS_OK server task created, or server already running
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to create server task
 */
esp_amp_rpc_status_t esp_amp_rpc_server_run(void);


/**
 * Stop the rpc server
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully stopped the rpc server
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to stop the rpc server
 */
esp_amp_rpc_status_t esp_amp_rpc_server_stop(void);


/**
 * Deinitialize the rpc server
 *
 * @retval ESP_AMP_RPC_STATUS_OK successfully deinit the rpc server
 * @retval ESP_AMP_RPC_STATUS_FAILED failed to deinit the rpc server
 */
esp_amp_rpc_status_t esp_amp_rpc_server_deinit(void);

#ifdef __cplusplus
}
#endif
