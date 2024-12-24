# Port layer

# Overview

In this document, we introduce the organization of port layer and list APIs that can be called from upper layers.

## Design

### Organization

ESP-AMP port layer exposes two subsets of APIs to upper layers: platform APIs and environment APIs.

* Platform APIs abstract the differences between different SoCs. Such APIs follow the naming convention starting with `esp_amp_platform`.
* Environment APIs implement functionalities that are specific to each software environment. Such APIs are indicated by their prefix `esp_amp_env`. At present, FreeRTOS and bare-metal are supported.

## Usage

In this section, we list APIs that can be called from upper layers.

### Platform APIs

#### Timestamp

The following API is provided to obtain the current timestamp in milliseconds since boot.

``` c
uint32_t esp_amp_platform_get_time_ms(void);
```

#### Delay

The following APIs are provided to perform busy-waiting delay in bare-metal environment. It is not recommended to use these APIs in isr or in OS environment.

``` c
void esp_amp_platform_delay_us(uint32_t time);
void esp_amp_platform_delay_ms(uint32_t time);
```

#### Interrupt

The following APIs are provided to enable/disable interrupts globally.

``` c
void esp_amp_platform_intr_enable(void);
void esp_amp_platform_intr_disable(void);
```

These two APIs are not recommended to be called in isr. If you need to create a critical section, please refer to the [critical section](#critical-section).

#### Software Interrupt

The following APIs are useful when you only need to disable software interrupt without disabling external interrupts such as peripheral interrupts.

``` c
void esp_amp_platform_sw_intr_enable(void);
void esp_amp_platform_sw_intr_disable(void);
```

### Environment APIs

#### Critical Section

To create a critical section, you can call `esp_amp_env_enter_critical()` and `esp_amp_env_exit_critical()`.

``` c
void esp_amp_env_enter_critical(void);
void esp_amp_env_exit_critical(void);
```
Nesting critical section is supported. These two APIs keep a count of the nesting depth. The critical section is exited when the nesting depth reaches 0. Application developer need to make sure that the number of `esp_amp_env_enter_critical()` calls is equal to the number of `esp_amp_env_exit_critical()` calls.

#### ISR

To check if the current context is in isr or not, you can call `esp_amp_env_in_isr()`.

``` c
int esp_amp_env_in_isr(void);
```

This API can be useful if you want to implement a function that can be called from both isr and task context. For example, ESP-AMP RPMsg device support both polling and interrupt mode. In polling mode, endpoint-registered callback is called from task context, while in interrupt mode, callback is called from isr. To make the callback function portable, you can use `esp_amp_env_in_isr()` to check the context, as shown below.

``` c
int IRAM_ATTR ept_cb(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    if (esp_amp_env_in_isr()) {
        ESP_DRAM_LOGI(TAG, "Interrupt-based callback invoked in ISR context");
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        QueueHandle_t msgQueue = (QueueHandle_t) rx_cb_data;
        if (xQueueSendFromISR(msgQueue, &msg_data, &xHigherPriorityTaskWoken) != pdTRUE) {
            esp_amp_rpmsg_destroy(&rpmsg_dev, msg_data);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        ESP_LOGI(TAG, "Polling-based callback invoked in non-ISR context");
        QueueHandle_t msgQueue = (QueueHandle_t) rx_cb_data;
        if (xQueueSend(msgQueue, &msg_data, pdMS_TO_TICKS(10)) != pdTRUE) {
            esp_amp_rpmsg_destroy(&rpmsg_dev, msg_data);
        }
    }
    return 0;
}
```
