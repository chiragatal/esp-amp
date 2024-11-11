| Supported Targets | ESP32-C6 | ESP32-P4 |
| ----------------- | ----- | ----- |

# ESP-AMP RPMsg

This example demonstrates how to use ESP-AMP RPMsg to send messages between endpoints on maincore and endpoint on subcore.

## How to use example

Run the following command to build and flash:

``` shell
source ${IDF_PATH}/export.sh
idf.py set-target <target>
idf.py build
idf.py flash monitor
```

You should see the following output:

``` shell
Main core started!
Demonstrating interrupt-based RPMsg handling on maincore
EPT0: Message from Sub core ==> Normal Msg: 0
Generating 933 x 743 = 693219
```

### Change the default message handling behavior of RPMsg

RPMsg supports interrupt-based and polling-based message handling. This example demonstrates both. By default, it uses interrupt-based message handling.

You can change the behavior on either side by modifying `EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_MAINCORE` and `EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE` in `sdkconfig`.
