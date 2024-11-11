| Supported Targets | ESP32-C6 | ESP32-P4 |
| ----------------- | ----- | ----- |

# ESP-AMP Software Interrupt

This example demonstrates how ESP-AMP software can perform Inter-Processor Call(IPC) from one core to another.

Maincore can trigger software interrupt to subcore. If corresponding interrupt handler is registered by the subcore, certain task defined by the handler will be performed by subcore.


## How to use example

``` shell
source ${IDF_PATH}/export.sh
idf.py set-target <target>
idf.py build
idf.py flash monitor
```
