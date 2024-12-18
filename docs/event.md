# Event

ESP-AMP Event component is designed to support bidirectional cross-core event notification. This document describes the design of ESP-AMP event and how to use it.

## Overview

Similar to event group and task notify in FreeRTOS, ESP-AMP Event is yet another bitmap-based synchronization mechanism, aiming for light-weight cross-core notification and synchronization. Compared to FreeRTOS queue and semaphore which are queue-based synchronization mechanism, the targeted use cases can be different. The following table lists the comparison between ESP-AMP Event and other synchronization primitives.

| Use cases | Queue | Semaphore | EventGroup | TaskNotify | ESP-AMP event | Note |
| --- | --- | --- | --- | --- | --- | --- |
| Broadcasting to more than one tasks | x | x | ✓ | x | ✓ | ESP-AMP event is based on EventGroup, so it is possible to broadcast to more than one task. |
| Record event order and how many times a single event triggered | ✓ | ✓ | x | x | x | Queue-based primitives can buffer events. Receiver of events can tell which event comes first and how many times an event is triggered. In bitmap-based EventGroup, TaskNotify and ESP-AMP event, it is impossible to tell which event comes first and which comes later. e.g. If WIFI_CONN and WIFI_DISCONN are set at the same time, it is impossible to tell the current WIFI status. It is also impossible to tell how many times WIFI_CONN is set. |
| Put sender in blocked state and wait for a send to complete | ✓ | ✓ | x | x | x | Queue-based primitives can buffer unprocessed events or block the sender until one or more pending events are processed. EventGroup, TaskNotify as well as ESP-AMP event cannot not block the sender until the receiver has processed the pending bits. |


## Design

The fundamental data structure of ESP-AMP event is a 32-bit atomic integer allocated from SysInfo. Its value indicates the pending events triggered by the remote core but not yet handled by local application. Local application can wait or clear the pending events. All operations on ESP-AMP event are atomic to avoid race conditions.

Each ESP-AMP event indicates a single-directional notification, either from maincore to subcore or vice versa, but not both. Users must avoid sending notification via a single ESP-AMP event from both sides. If you want to achieve bi-directional synchronization, you should create two ESP-AMP events.

### Delivery of a Cross-Core Event

Each ESP-AMP event consists of an atomic integer allocated from SysInfo indicating the pending events set by the notifying core. Notifying core sets the corresponding bits to the atomic integer by atomic OR operations `atomic_fetch_or()`. To deliver the event to the waiting core, software interrupt or polling mechanism can be used on the waiting core, depending on the environment is whether FreeRTOS or baremental. In bare-metal environment, main loop polls for the value of the atomic integer and checks whether the events are set by atomic operation `atomic_cmp_exchange()`. In FreeRTOS environment, it is not recommended to poll the atomic integer in a tight loop. Instead, FreeRTOS can suspend the task blocked on certain events and only resume it later when the wait condition is met. ESP-AMP suspends tasks by calling `xEventGroupWaitBits()` internally and notifies FreeRTOS to wake up tasks blocked on events by calling `xEventGroupSetBitsFromISR(event_handle, bit_mask)` in the ESP-AMP event ISR triggered by the notifying core. To notify the corresponding FreeRTOS event handle associated with the underlying ESP-AMP event, ESP-AMP event must be bound to FreeRTOS event handle.

### Bind ESP-AMP Event To FreeRTOS Event Handle

As mentioned in the previous section, ESP-AMP event must be bounded to a FreeRTOS event handle so that when ESP-AMP event ISR is triggered by notifying core, FreeRTOS can correctly wake up the tasks blocked on the ESP-AMP event. That's why this `esp_amp_event_bind_by_id()` API comes into existence.

We introduce an seperate `esp_amp_event_bind_by_id()` API to decouple the ESP-AMP event allocation from OS-specific event handle creation. This can address the timing constraints during system initialization. ESP-AMP event must be allocated using `esp_amp_sysinfo_alloc()`, which must take place when maincore sets up the sysinfo data structure. This is a very early stage, before subcore starts to run and tasks on maincore are created. However, it is quite common to create FreeRTOS event handles in tasks. In this case, `esp_amp_event_bind_by_id()` API gives the flexibility to postpone the creation of event handles.

`xEventGroupSetBits()` is called at the exit of `esp_amp_event_bind_by_id()`, to avoid missing events which can be set before binding is created.

### Reserved Events

ESP-AMP event component reserves two events for internal use, one for maincore to notify subcore, and one for subcore to notify maincore. Currently only `SUBCORE_READY_EVENT` is used for subcore to notify maincore that it is ready to execute.

## Usage

### Create ESP-AMP events

To create ESP-AMP events, use the following API in maincore application before subcore application is started:

``` c
int esp_amp_event_create(uint16_t sysinfo_id);
```

* `sysinfo_id` is the ID of the SysInfo object representing the ESP-AMP event.

This API can only be called from maincore before subcore application is started. Since SysInfo cannot be freed once it is allocated, there is no API to destroy an ESP-AMP event.

### Bind/Unbind ESP-AMP Event to FreeRTOS Event Handles

To bind/unbind an ESP-AMP event to FreeRTOS EventGroup handle, use the following APIs:

``` c
int esp_amp_event_bind_handle(uint16_t sysinfo_id, void *event_handle);
int esp_amp_event_unbind_handle(uint16_t sysinfo_id);
```

* `sysinfo_id` is the ID of the SysInfo object representing the ESP-AMP event.
* `event_handle` is the FreeRTOS EventGroup handle to be bound to the ESP-AMP event.

This API is for FreeRTOS environment only. 

Any ESP-AMP event created in FreeRTOS must be bound to exactly one FreeRTOS EventGroup handle. This API internally checks if the ESP-AMP event denoted by `sysinfo_id` already bounded to an existing FreeRTOS EventGroup handle. If yes, the new FreeRTOS EventGroup handle will replace the existing one.

Please ensure that any ESP-AMP event used in FreeRTOS is successfully bounded to FreeRTOS EventGroup handle. If not, tasks blocked on the ESP-AMP event will never be woken up. Nevertheless, this does not mean binding process on local core must complete before notifying happens on remote core. Bind API always calls FreeRTOS `xEventGroupSetBits()` before exit to wake up tasks potentially blocked by remote events that are notified before binding process is completed.

### Notify Events to Remote Core

The following API can be used to notify an event to the remote core.

``` c
uint32_t esp_amp_event_notify_by_id(uint16_t sysinfo_id, uint32_t bit_mask);
```

* `sysinfo_id` is the ID of the SysInfo object representing the ESP-AMP event.
* `bit_mask` is the bit mask of the event bits to be notified. Upper 8 bits are reserved by FreeRTOS. Use lower 24 bits only.

We also offer convenient macros to notify an event to the remote core via reserved event. The definition of the macros is as follows:

``` c
#if IS_MAIN_CORE
#define esp_amp_event_notify(bit_mask) \
    esp_amp_event_notify_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, bit_mask)
#else
#define esp_amp_event_notify(bit_mask) \
    esp_amp_event_notify_by_id(SYS_INFO_RESERVED_ID_EVENT_SUB, bit_mask)
#endif /* IS_MAIN_CORE */
```

### Wait Events in FreeRTOS Environment

You can use `xEventGroupWaitBits()` to wait for the event bits set by remote core. You can also use the following API when the ESP-AMP event is bounded to FreeRTOS EventGroup handle.

``` c
uint32_t esp_amp_event_wait_by_id(uint16_t sysinfo_id, uint32_t bit_mask, bool clear_on_exit, bool wait_for_all, uint32_t timeout);
```

* `sysinfo_id` is the ID of the SysInfo object representing the ESP-AMP event.
* `bit_mask` is the bit mask of the event to be waited. Upper 8 bits are reserved by FreeRTOS. Use lower 24 bits only.
* `clear_on_exit` is a boolean value indicating whether the bits in `bit_mask` should be cleared when the wait condition is met (if the function returns for a reason other than timeout).
* `wait_for_all` is a boolean value indicating whether all bits in `bit_mask` must be set or any bit in `bit_mask` must be set.
* Return value is the bit mask of the event before the event is cleared.

Following macros are provided as a more convenient way of using `esp_amp_event_wait_by_id()` via reserved ESP-AMP event.

``` c
#if IS_MAIN_CORE
#define esp_amp_event_wait(bit_mask, clear_on_exit, wait_for_all, timeout) \
    esp_amp_event_wait_by_id(SYS_INFO_RESERVED_ID_EVENT_SUB, bit_mask, clear_on_exit, wait_for_all, timeout)
#else
#define esp_amp_event_wait(bit_mask, clear_on_exit, wait_for_all, timeout) \
    esp_amp_event_wait_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, bit_mask, clear_on_exit, wait_for_all, timeout)
#endif /* IS_MAIN_CORE */
```

If multiple tasks are waiting for the same event, set `clear_on_exit` to true if you want to clear the bit mask when the wait condition is met. Be careful when you have to manually call `esp_amp_event_clear_by_id()` which can lead to race condition.

### Wait/Poll Events in Bare-metal Environment

`esp_amp_event_wait_by_id()` can be used to put the caller into busy-waiting in bare-metal environment. If polling instead of busy-waiting is prefered, `esp_amp_event_poll_by_id()` can be used instead. It is simply a wrapper of `esp_amp_event_wait()` with `timeout=0`. 

``` c
#if IS_ENV_BM
#define esp_amp_event_poll_by_id(sysinfo_id, bit_mask, wait_for_all, clear_on_exit) \
    esp_amp_event_wait_by_id(sysinfo_id, bit_mask, true, true, 0)
#endif /* IS_ENV_BM */
```

Setting `clear_on_exit=true` to avoid race condition. 

If `clear_on_exit` is set to `true`, any bits within `bit_mask` will be cleared **ONLY** when the wait condition is met (if the function returns for a reason other than timeout). If the return reason is timeout, the bits in `bit_mask` will not be cleared.

### Avoid Calling `notify()` And `clear()` From The Same Core

If setbits and clearbits are allowed on the same core, it is impossible for the other core to tell which bits are set and which bits are cleared, as well as the order of setbits and clearbits.

To tell which bits are set and which are cleared, we can use two separated bitmaps, set-bitmap and clear-bitmap. However, merely two bitmaps cannot tell which event comes first. If same bits are set in both set-bitmap and clear-bitmap without any sequence information, it can be interpreted in two distinct ways: If set-first-then-clear, the event should is cleared. If clear-first-then-set, the event is set.

Instead of using two separate bitmaps which leads to ambiguity, a single bitmap can indicate which bits are set currently. However, although we can tell which bits are set, it is impossible to know which bits are cleared. This holds true for both bare-metal and FreeRTOS.

On FreeRTOS, however, each time FreeRTOS setbits, it will check if any task is blocked on these newly set bits. If so, it will wake up the task by moving it from blocked list to ready list. However, this is impossible in bare-metal environment. Once a bit is set, the task is determined to wake up. There is no missing event.

To coordinate between two cores, we choose to interprete the bitmap as pending events. If a bit is set, it means the event is set by notifier but not processed by the receiver. Once the receiver process the event, the bit must be cleared at once. In this sense, it works pretty much like the signal in linux.

### Avoid Missing Events

If `wait()` and `clear()` are not performed in atomic manner, chances are that remote core sets bits right after `wait()` and before `clear()`. To avoid missing any event like this, it is suggested to set `clear_on_exit=true` when using `wait()`/`poll()` if you want to clear the bits in `bit_mask` when the wait condition is met.

### Sdkconfig Options

* `CONFIG_ESP_AMP_EVENT_TABLE_LEN`: Number of OS-specific event handles ESP-AMP events can bind to (excluding reserved ones).

## Application Example

* [event](../examples/event): demonstrates how to create ESP-AMP events and use them to synchronize between two cores.
