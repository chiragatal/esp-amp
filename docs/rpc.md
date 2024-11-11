# RPC

## Overview

RPC is a protocol that enables a client to initiate a subroutine on a server, which is running on a different address space. The server will execute the subroutine and return the result to the client. ESP AMP implements a simple RPC client and server. This document introduces the design and usage of ESP-AMP RPC component.

## Design

RPC client and server in ESP-AMP are built on top of RPMsg component. Similar to any RPMsg endpoint, RPC client and server register their own rx callback (`ept_rx_cb`) to process incoming messages. This callback function can be invoked automatically or manually when there is any incoming message destined to the endpoint.

ESP-AMP RPC client works as a proxy encapsulating the complexity of managing multiple pending RPC requests from other tasks. RPC client keeps sending requests to RPC server and wait for response. Pending request is an RPC request that has sent out to server but not yet returned back. A pending list is used to keep track of all pending requests. Each RPC request is assigned with a unique request ID. Once the request is executed by RPC server, server sends back a response with the same request ID. When RPC client receives the response, it uses the request ID to remove the request from pending list.

Running on a different core from RPC client, ESP-AMP RPC server keeps receiving incoming RPC requests from client and dispatch to the corresponding service handlers. After RPC request is executed, the result is then sent back to client. The diagram below demonstrates the workflow of RPC server to receive and execute a request.

## Usage

### Initialization

Each client or server is assigned with unique endpoint ID and destination ID. To create a pair of client and server communicating with each other, their endpoint ID must be each other's destination ID. The following code creates an RPC client on maincore and an RPC server on subcore matched with each other.

``` c
// on maincore
esp_amp_rpc_client_init(&rpmsg_maincore_dev, RPC_MAIN_CORE_CLIENT /* endpoint */, RPC_SUB_CORE_SERVER /* destination */);

// on subcore
esp_amp_rpc_server_init(&rpmsg_subcore_dev, RPC_MAIN_CORE_CLIENT /* destination */, RPC_SUB_CORE_SERVER /* endpoint */);

```

Recall that RPC client and server are essentially RPMsg endponts. In FreeRTOS environment, interrupt-based messaging mechanism can be enabled by `esp_amp_rpmsg_intr_enable()`, and the corresponding callback function can be automatically invoked when there is incoming message. Applications in bare-metal environment must manually call `esp_amp_rpmsg_poll()` for polling the incoming message. Refer to [RPMsg](./rpmsg.md) for more details.

### Serialization

ESP RPC does not either implement or integrate serialization library. Users are free to choose their favorite serialization library to construct RPC packets. ESP-AMP RPC expose its packet buffer point to user applications. For example, `memcpy()` can be used as a simple encoder to construct RPC packet, and a simple decoder to extract RPC result from transport buffer.

### RPC Client

A complete RPC client workflow consists of the following steps:
1. Create RPC request: construct RPC request payload and push it to pending list. (User-defined encoder is required)
2. Send RPC request: send RPC request to server.
3. Process RPC response: extract RPC result. (User-defined decoder is required)
4. Clean up RPC request: remove RPC request from pending list.

#### 1. Create RPC Request

You can create an RPC request with the following API in both FreeRTOS and bare-metal environment:

``` c
esp_amp_rpc_req_handle_t esp_amp_rpc_client_create_request(uint16_t service_id, void *params, uint16_t params_len);
```

* service_id: service ID of the RPC request. This service ID is used to execute service handler on the server side.
* params: pointer to a constructed RPC request payload. Internally `memcpy()` is used to copy the payload to transport buffer.
* params_len: length of the constructed RPC request payload.

#### 2. Send RPC Request

In FreeRTOS environment, RPC request is sent out in synchronous syntax. The following API accepts a timeout value in milliseconds. Sending task will be blocked until the RPC response is received or timeout.

``` c
esp_amp_rpc_status_t esp_amp_rpc_client_execute_request(esp_amp_rpc_req_handle_t req, void **param_out, int *param_out_len, uint32_t timeout_ms);
```

* req: handle of the RPC request.
* param_out: pointer to the RPC response payload. You can use `memcpy()` or use any decoder to extract the RPC response from the transport buffer.
* param_out_len: length of the RPC response payload.
* timeout_ms: maximum time to wait for the RPC response.

In bare-metal environment, RPC request is sent out in asynchronous syntax. The following API accepts a callback function to be invoked when the RPC response is received.

``` c
esp_amp_rpc_status_t esp_amp_rpc_client_execute_request_with_cb(esp_amp_rpc_req_handle_t req, esp_amp_rpc_req_cb_t cb, uint32_t timeout_ms);
```

* req: handle of the RPC request.
* cb: callback function to be invoked when the RPC response is received.
* timeout_ms: maximum time to wait for the RPC response.

#### 3. Process Result

In FreeRTOS environment, task sending RPC request is blocked until the RPC response is received or timeout. Address and size of RPC response payload can be obtained from arguments of `esp_amp_rpc_client_execute_request()`. See the previous section for more details.

In bare-metal environment, callback function is invoked when the RPC response is received or timeout. The callback function has the following prototype:

``` c
typedef void (* esp_amp_rpc_req_cb_t)(esp_amp_rpc_status_t status, void *params_out, uint16_t params_out_len);
```

* status: status of the RPC response.
* params_out: pointer to the payload of the RPC response. You can use `memcpy()` or any decoder to extract the RPC response from the transport buffer.
* params_out_len: length of the payload of the RPC response.

The possible status of the RPC response is listed below.

``` c
  ESP_AMP_RPC_STATUS_OK = 0, /* success */
  ESP_AMP_RPC_STATUS_NO_SERVICE, /* service id not found by server*/
  ESP_AMP_RPC_STATUS_EXEC_FAILED, /* execution failed on server */
  ESP_AMP_RPC_STATUS_TIMEOUT /* timeout */
```

If status is `ESP_AMP_RPC_STATUS_OK`, the RPC request is executed successfully. Decoder can be used to extract the RPC result from `params_out` and `params_out_len`. Otherwise, these two values are invalid.

#### 4. Clean up RPC Request

In FreeRTOS environment, after RPC result is processed, the following API must be called to clean up the resources allocated for the request.

``` c
void esp_amp_rpc_client_destroy_request(esp_amp_rpc_req_handle_t req);
```

* req: handle of the RPC request.

In bare-metal environment, timeout request must be manually cleaned up by calling the following API periodically.

``` c
void esp_amp_rpc_client_complete_timeout_request(void);
```

### RPC Server

A complete RPC server workflow consists of the following steps:
1. Register service handlers: register service handlers to RPC server.
2. Receive RPC request: in FreeRTOS environment, RPC requests are received by interrupt. In bare-metal environment, RPC requests are received by polling.
3. Execute RPC request: execute RPC request by calling the corresponding service handler.
4. Reply to RPC client: send execution result to client. This step is handled by the RPC server internally.

#### 1. Register Service Handlers

The following API can be used to register service handlers in both FreeRTOS and bare-metal environment.

``` c
esp_amp_rpc_status_t esp_amp_rpc_server_add_service(esp_amp_rpc_service_id_t srv_id, esp_amp_rpc_service_func_t srv_func);
```
* srv_id: service ID of the RPC request.
* srv_func: service handler.

The prototype of service handler is as follows:

``` c
esp_amp_rpc_status_t rpc_service_add(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
```

* params_in: pointer to the payload of the RPC request.
* params_in_len: length of the payload of the RPC request.
* params_out: pointer to the payload of the RPC response.
* params_out_len: length of the payload of the RPC response.
* Return value: execution status. `ESP_AMP_RPC_STATUS_OK` if the RPC request is executed successfully. `ESP_AMP_RPC_STATUS_EXEC_FAILED` if execution failed. `ESP_AMP_RPC_STATUS_BAD_PACKET` if the RPC request packet is invalid or params_out buffer is too short to fit the RPC result.

#### 2. Receive RPC Request

In FreeRTOS environment, server-registered rx callback is invoked once there is a incoming packet destined for the server endpoint. Packets are then forwarded from interrupt context to RPC server process task via FreeRTOS queue. The size of queue is set to be `CONFIG_ESP_AMP_RPC_MAX_PENDING_REQ`. If the queue is full, the packet will be dropped. Client will receive timeout error.

In bare-metal environment, RPC requests are manually received by polling. RPC server polls for incoming packets and processes them one by one.

#### 3. Execute RPC Request

The following code demostrates an example of service handler:

``` c
esp_amp_rpc_status_t rpc_service_add(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    add_params_in_t *add_params_in = (add_params_in_t *)params_in;
    add_params_out_t *add_params_out = (add_params_out_t *)params_out;

    add_params_out->ret = add(add_params_in->a, add_params_in->b);
    *params_out_len = sizeof(add_params_out_t);

    return ESP_AMP_RPC_STATUS_OK;
}
```

### Sdkconfig Options

* `CONFIG_ESP_AMP_RPC_MAX_PENDING_REQ`: RPC client can process up to this number of incoming pending requests. By default, this value is set to 4, which means at most 4 tasks can send requests and wait for response in the meantime. Increase this value will allow more pending requests but also introduce more memory footprint.
* `CONFIG_ESP_AMP_RPC_SERVICE_TABLE_LEN`: RPC server can serve up to this number of services. Make sure this value is larger than the number of services RPC server performs.


## Application Examples

* [maincore_client_subcore_server](../examples/rpc/maincore_client_subcore_server): demonstrates how to initiate an RPC client in FreeRTOS environment on maincore side and an RPC server in bare-metal environment on subcore side.
* [subcore_client_maincore_server](../examples/rpc/subcore_client_maincore_server): demonstrates how to initiate an RPC client in bare-metal environment on subcore side and an RPC client in FreeRTOS environment on maincore side.
