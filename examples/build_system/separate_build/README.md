| Supported Targets | ESP32-C6 | ESP32-P4 |
| ----------------- | ----- | ----- |

# ESP-AMP Separate Build

This example demonstrates how to build ESP-AMP project separately for maincore target as well as subcore target.

## How to use example

### First build and flash the maincore firmware

``` shell
cd maincore_project
idf.py set-target <target>
idf.py build
idf.py flash
```

### Then build and flash the subcore firmware

``` shell
cd subcore_project
idf.py set-target <target>
idf.py build

# manually download subcore image to flash
esptool.py write_flash 0x200000 build/subcore_separate_build.bin
```

### Verify the subcore firmware is running

Run `idf.py monitor` to see the subcore firmware is running. The output should be similar to:

``` shell
I (395) app_main: Loading subcore firmware from partition
I (405) esp-amp-loader: Reserved dram region (0x40874610 - 0x40878610) for subcore
I (415) esp-amp-loader: Give unused reserved dram region (0x40877c5c - 0x40878610) back to main-core heap
I (425) app_main: Subcore started successfully
I (425) main_task: Returned from app_main()
```

### Explanation

A customized partition table `partitions.csv` is used in this example. Compared with any other normal ESP-IDF single-app application, a new entry is added to store the subcore firmware.

``` csv
# Name,      Type,       SubType,    Offset,      Size
sub_core,    data,       0x40,       0x200000,    16K
```

Make sure when you download the subcore firmware using `esptool.py write_flash`, the offset is 0x200000. The generated subcore firmware is stored in `subcore_project/build/${subcore_project_name}.bin`.
