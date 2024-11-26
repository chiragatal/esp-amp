| Supported Targets | ESP32-C6 | ESP32-P4 |
| ----------------- | ----- | ----- |

# ESP-AMP Unified Build Example

ESP-AMP build system supports compiling both maincore firwmare and subcore firmware simultaneously in a single build. The generated subcore firmware can be embedded into `.rodata` section of maincore firmware or downloaded to a dedicated flash partition. All these operations can be done by a single `idf.py build flash` command.

This example demonstrates the following features:
* Build ESP-AMP project using unified build system
* Embed subcore firmware into maincore firmware and load it at runtime
* Download subcore firmware to a dedicated flash partition and load it at runtime

## Quick start

You can build and flash the example by running the following command:

```shell
export ESP_AMP_PATH=<path/to/esp-amp>
source ${IDF_PATH}/export.sh
idf.py set-target <target>
idf.py build
idf.py flash monitor
```

From the output of `idf.py monitor`, you should see the following output:

``` shell
I (395) app_main: Loading subcore firmware from embedded binary
I (405) esp-amp-loader: Reserved dram region (0x40874610 - 0x40878610) for subcore
I (415) esp-amp-loader: Give unused reserved dram region (0x40877c5c - 0x40878610) back to main-core heap
I (425) app_main: Subcore started successfully
I (425) main_task: Returned from app_main()
```

The first line indicates that the subcore firmware is loaded from embedded binary or flash partition. The second and third lines indicate that the unused reserved dram region is given back to main-core heap.

The default way of placing the subcore firmware is by embedding it into maincore firmware. You can change this behavior by changing `CONFIG_SUBCORE_FIRMWARE_LOCATION` from `SUBCORE_FIRMWARE_EMBEDDED` to `SUBCORE_FIRMWARE_PARTITION` via `idf.py menuconfig`.

## How to use example

### Structure

``` shell
├── CMakeLists.txt # ESP-AMP project-level CMakeLists.txt
├── maincore
│   ├── CMakeLists.txt # maincore component-level CMakeLists.txt
│   └── main
│       └── app_main.c
└── subcore
    ├── CMakeLists.txt # subcore project-level CMakeLists.txt
    ├── subcore_project.cmake # defines SUBCORE_APP_NAME and SUBCORE_PROJECT_DIR needed by unified build system
    └── main
        ├── CMakeLists.txt # main component CMakeLists.txt of subcore project
        └── main.c
```

* top-level `CMakeLists.txt` in the root directory is the ESP-AMP project CMakeLists.txt. It initiates the ESP-AMP build system and adds maincore and subcore components.
* `maincore/CMakeLists.txt` is the maincore component CMakeLists.txt. `esp_amp_add_subcore_project()` can be used here to initiate subcore build process and add subcore firmware to maincore firmware.
* `subcore_project.cmake` must be manually included in the project's top level CMakeLists.txt before `project()`. It defines `SUBCORE_APP_NAME` and `SUBCORE_PROJECT_DIR` needed by unified build system.


### Change subcore firmware partition

To change the flash partition of subcore firmware, follow the steps below:

#### 1. add new partition for subcore firmware in `partitions.csv`

you can change the partition name, type, subtype, offset and size to whattever you want. In this README, we choose `sub_core` as the partition name, `data` as the partition type, `0x40` as the partition subtype, `0x200000` as the partition offset, and `16K` as the partition size.

``` shell
# name,      type,       subtype,    offset,      size
sub_core,    data,       0x40,       0x200000,    64K,
```

#### 2. call `esp_amp_add_subcore_project` to add subcore project to maincore project

The prototype of `esp_amp_add_subcore_project` is as follows:

``` shell
esp_amp_add_subcore_project(subcore_app_name subcore_project_dir partition_type partition_subtype [EMBED])
```

* `subcore_app_name`: name of subcore app
* `subcore_project_dir`: path of subcore project
* `partition_type`: type of subcore firmware
* `partition_subtype`: subtype of subcore firmware
* `[EMBED]`: flag to embed subcore firmware into maincore firmware

#### 3. load the subcore firmware at runtime

In maincore firmware, load the subcore firmware at runtime by calling `esp_amp_load_sub_from_partition`. This function accepts a pointer to `esp_partition_t` as input. The `esp_partition_t` can be obtained by calling `esp_partition_find_first` function.

``` c
const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
```
