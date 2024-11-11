| Supported Targets | ESP32-C6 | ESP32-P4 |
| ----------------- | ----- | ----- |

# ESP-AMP Event

This example demonstrates how ESP-AMP event can synchronize two cores.

One core can wait for a specific event which is supposed to be triggered by another core.


## How to use example

``` shell
source ${IDF_PATH}/export.sh
idf.py set-target <target>
idf.py build
idf.py flash monitor
```
