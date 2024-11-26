# ESP-AMP Build System

This document introduces the build system of ESP-AMP, where to store subcore firmware and how to load subcore firmware into DRAM.

## Overview

An ESP-AMP project generates two separated firmwares for maincore target and subcore target respectively. Maincore firmware follows the standard IDF buildflow. Subcore firmware is built by an external CMake project with its own toolchain settings. After building, subcore firmware can be embedded into maincore firmware as a binary blob or stored in a separate flash partition. It then can be loaded to DRAM by maincore firmware for subcore to execute.

## Building ESP-AMP Projects

ESP-AMP provides two different approaches to build ESP-AMP projects, unified build and separated build. They fit in different use cases.

* Unified build:
  * Build both maincore and subcore firmware in a single command, ideal for collaborative development.
  * Suitable for users who are new to ESP-AMP and want to quickly get started with the framework.
  * Easy to integrate into unit tests. For more details, refer to [README.md](test_apps/esp_amp_basic_tests/README.md) under test_apps/esp_amp_basic_tests.
  * Embedding subcore firmware into maincore firmware is supported. Partition storage is also supported.
  * Subcore firmware OTA can be achieved via standard IDF OTA mechanism if subcore is embedded, with maincore firmware unchanged but only embedded subcore firmware updated. No special OTA mechanism is needed.

* Separated build:
  * Build each firmware separately, allowing for individual development for each core.
  * Designed for use cases where subcore firmware must be developed independently from maincore firmware. For example, maincore firmware is running prebuilt proprietary image and subcore firmware is developed by a third party.
  * Embedding subcore firmware into maincore firmware is not supported. Subcore firmware must be downloaded to flash partition manually.

### Prerequisites

Before you build the subcore firmware, make sure you have gone through the following checklists without any issue.
1. Make sure maincore and subcore are built with the same sys_info and event settings.
2. Make sure the sdkconfig options are the same for maincore and subcore. For unified build, this is done automatically.
3. Refer to documents [memory_layout](./memory_layout.md) and [shared_memory](./shared_memory.md) to configure memory space for maincore and subcore correctly.
4. If you choose ESP32P4 with HP core as subcore, `CONFIG_FREERTOS_UNICORE` must be selected via sdkconfig. This is done automatically if `CONFIG_ESP_AMP_SUBCORE_TYPE_HP` is chosen.

### Unified Build

A typical ESP-AMP project with unified build follows the structure below:

```
├── maincore                            # maincore component folder
│   ├── main
│   │   └── app_main.c
│   └── CMakeLists.txt
├── subcore                             # subcore project folder
│   ├── main                            # subcore main component folder
│   │   ├── main.c
│   │   └── CMakeLists.txt              # subcore main component CMakeLists.txt
│   ├── CMakeLists.txt                  # subcore project CMakeLists.txt
│   └── subcore_config.cmake            # subcore project config file
├── common                              # common header files needed by both maincore and subcore
├── partitions.csv                      # partition table
├── sdkconfig.defaults                  # top-level (maincore & subcore) project sdkconfig.defaults
├── CMakeLists.txt                      # top-level (maincore) project CMakeLists.txt
└── build                               # top-level (maincore & subcore) project build folder
    └── subcore                         # subcore build folder
```

* In unified build, the top-level project is the maincore project. As any other normal IDF projects, it contains a project-level `CMakeLists.txt` file, a `partitions.csv` file and a `sdkconfig.defaults` file. Subcore is an external cmake project initiated by the maincore project.
* The `maincore` folder is indeed a renamed main component of maincore project (refer to IDF's doc [build system](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#renaming-main-component) for more details). Renaming it to maincore is to distinguish it from the subcore folder. The maincore component contains a component-level `CMakeLists.txt` file where `esp_amp_add_subcore_project()` function can be invoked to initiate the build process of subcore project.
* Unlike the maincore folder, the `subcore` folder is not a component folder. Instead, it contains project-level `CMakeLists.txt` which tells CMake how to build subcore firmware. In unified build mode, the name(`SUBCORE_APP_NAME`) and the path(`SUBCORE_PROJECT_DIR`) of subcore project must be specified before `project()` in the top-level project `CMakeLists.txt` is invoked. We suggest defining these variables in the subcore config file `subcore_config.cmake` and include it in the top-level project `CMakeLists.txt`.
* The `common` folder contains variables to be shared between maincore and subcore application, such as sys_info id and event id.
* In `partitions.csv`, a new entry is created to store subcore firmware. Make sure the type and subtype defined here matches the parameters `TYPE` and `SUBTYPE` of `esp_amp_add_subcore_project()` function.
* The maincore firmware is generated under `build` folder. The subcore firmware is generated under `build/subcore` folder.
* sdkconfig.defaults is the top-level project sdkconfig.defaults. It contains the sdkconfig options for both maincore and subcore. More details of setting kconfig for subcore project components can be found [here](#configure-subcore-component)

### Separated Build

A typical ESP-AMP project with separated build follows the structure below:

```
├── maincore_project                    # maincore project folder
│   ├── main                            # maincore main component folder
│   │   ├── app_main.c                  # maincore main component source file
│   │   └── CMakeLists.txt              # maincore main component CMakeLists.txt
│   ├── CMakeLists.txt                  # maincore project-level CMakeLists.txt
│   ├── partitions.csv                  # maincore project partition table
│   └── sdkconfig.defaults              # maincore project sdkconfig.defaults
├── subcore_project                     # subcore project folder
│   ├── main                            # subcore main component folder
│   │   ├── main.c                      # subcore main component source file
│   │   └── CMakeLists.txt              # subcore main component CMakeLists.txt
│   ├── CMakeLists.txt                  # subcore project-level CMakeLists.txt
│   └── sdkconfig.defaults              # subcore project sdkconfig.defaults
├── common                              # common header files needed by both maincore and subcore
└── build                               # top-level project build folder
    └── subcore                         # subcore build folder
```

* maincore_project folder is the project folder for creating maincore firmware. As any other normal IDF projects, it contains a project-level `CMakeLists.txt` file, a `partitions.csv` file and a `sdkconfig.defaults` file.
* subcore_project folder is the project folder for creating subcore firmware. Different from unified build, `sdkconfig` options for two cores are not shared. Make sure the kconfig options working for both cores are set to the same value in `sdkconfig.defaults` of both subcore project and maincore project, especially ESP-AMP shared memory related ones.
* The `common` folder contains variables to be shared between maincore and subcore application, such as sys_info id and event id.
* In `maincore_project/partitions.csv`, an entry is created to store subcore firmware. Maincore firmware relies on this entry to locate subcore firmware. Make sure the subcore firmware is downloaded to this partition.

## Subcore Firmware Build Process

## Build Subcore Firmware

The subcore project to build subcore firmware can be initiated by any of the following ways:
* in separated build, subcore project can be initiated by calling `idf.py build` command in `subcore_project` folder.
* in unified build, `idf.py build` command in top-level project folder will build both firmwares. The build process of subcore firmware is initiated by maincore component calling cmake function `esp_amp_add_subcore_project()`. This function creates an external project to launch cmake build process of subcore project.

The prototype of `esp_amp_add_subcore_project()` function is as follows:

``` shell
esp_amp_add_subcore_project(app_name project_dir [EMBED] [PARTITION] [TYPE type] [SUBTYPE subtype])
```

* `app_name` is the name of the subcore application.
* `project_dir` is the path of the subcore project.
* `EMBED` is an optional parameter. If this parameter is specified, the subcore firmware will be embedded into maincore firmware.
* `PARTITION` is an optional parameter. If this parameter is specified, the subcore firmware will be downloaded into the partition specified by this parameter.
* `TYPE` and `SUBTYPE` are single value parameters. If `PARTITION` is specified, the subcore firmware will be downloaded into the partition specified by the partition table entry with type `TYPE` and subtype `SUBTYPE`.

We suggest creating a subcore config file `subcore_config.cmake` under subcore project folder, with two variables `SUBCORE_APP_NAME` and `SUBCORE_PROJECT_DIR` defined inside. Manually include it in the top-level project `CMakeLists.txt` before `project()` to make them globally available in the entire project. This way, when calling `esp_amp_add_subcore_project()` function in maincore project, `SUBCORE_APP_NAME` and `SUBCORE_PROJECT_DIR` can be used to specify the subcore project name and path.

### Configure Subcore Component

Subcore project also supports IDF's component feature. This means you can create mulitple components with `idf_component_register()` in each component folder. Components placed in `${CMAKE_PROJECT_DIR}/component` will also be automatically detected and built when you run `idf.py build` command.

idf component manager is also supported. This means you can create an `idf_component.yml` file under subcore project's main component folder to download dependencies from IDF component registry. For more information, please refer to [IDF Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

Subcore project also supports sdkconfig. However, `Kconfig` files in subcore project are handled differently in separated build and unified build. In separated build, subcore component `Kconfig` is processed on its own, without involvement of maincore. Separate `sdkconfig` file is generated for subcore project only when you run `idf.py set-target` in subcore project folder. In unified build, subcore sdkconfig doesnot generate `sdkconfig`file. Instead, itrelies on maincore's project to generates a commonly used `sdkconfig` file shared by both projects. Details are described in the following sections.

**NOTE**: subcore project does not support renamed main component. Keep the main component name as `main`.

#### Separated Build

In separated build, a separate `sdkconfig` file for subcore project is generated when `idf.py build` command is called in subcore project folder.

#### Unified Build

In unified build, subcore and maincore share the same `sdkconfig` file. When running `idf.py set-target` and `idf.py menuconfig` in the top-level project folder, a single sdkconfig file with config options from both maincore and subcore is generated. This feature requires manually adding subcore components to maincore's `EXTRA_COMPONENT_DIRS` to trigger sdkconfig generation for subcore components. To avoid actual build of subcore components in maincore project, we need to add a guard `if(NOT SUBCORE_BUILD)` in subcore project `CMakeLists.txt` when building maincore firmware, as shown below:

``` cmake
# example code from unified_build/subcore/components/sub_greeting/CMakeLists.txt
# skip building component in maincore project, only trigger sdkconfig generation
if(NOT SUBCORE_BUILD)
    idf_component_register()
    return()
endif()

# proceed here only when building subcore firmware
idf_component_register(
    SRCS "greeting.c"
    INCLUDE_DIRS "."
)
```

**NOTE:** To avoid subcore components overwritting maincore components with the same name, we suggest adding prefix `sub` to subcore component name.

Meanwhile, since subcore project does not support renamed main component, the main component must not be included in `EXTRA_COMPONENT_DIRS` of maincore project. This also means that `Kconfig` file in subcore main component can not be processed by maincore project. As a workaround, we can add a dummy component `sub_main_cfg` in component folder of subcore project, and add it to `EXTRA_COMPONENT_DIRS` of maincore project. More details can be found [here](../examples/build_system/unified_build/subcore/components/sub_main_cfg/README.md).

## Subcore Firmware Storage

After the subcore firmware is generated, it must be stored somewhere in flash so that it can be loaded by maincore firmware. Users can choose either to embed the subcore firmware into maincore firmware or download it into separate flash partition.

### Embed into Maincore Firmware

Feel free to skip this section if you choose separated build approach, since unified build approach is the only supported way to embed subcore firmware into maincore.

The embedding of subcore firmware is initiated by `target_add_binary_data(${COMPONENT_LIB} ...)` which is called inside `esp_amp_add_subcore_project()` after subcore firmware is generated. `COMPONENT_LIB` is the maincore component triggering `esp_amp_add_subcore_project()`. The subcore firmware is embedded into the static library of this component and later linked to the `.rodata` section of the eventual maincore executable. The maincore executable can refer to this embedded binary by `_binary_${SUBCORE_APP_NAME}_bin_start` and `_binary_${SUBCORE_APP_NAME}_bin_end`. Recall that `SUBCORE_APP_NAME` is the name of the subcore application defined in `subcore_config.cmake`.

### Store in Separate Flash Partition

To download the subcore firmware, add a new entry in `partitions.csv` in maincore project to store subcore firmware. Any partition types and subtypes are allowed.

In separated build mode, manually run `esptool.py write_flash <offset> build/${SUBCORE_APP_NAME}.bin` in subcore project to download the subcore firmware. Here the `offset` is the offset of subcore partition specified in maincore `partitions.csv`.

In unified build mode, you can call `esp_amp_add_subcore_project()` function with option `PARTITION` specified. Subcore firmware will be downloaded to flash along with maincore firmware when you run `idf.py flash`. Make sure `TYPE` and `SUBTYPE` are the same as the ones defined in top-level `partitions.csv`.

## Loading Subcore Firmware

ESP-AMP subcore firmware image follows the standard [App Image Format](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/app_image_format.html) of ESP32. ESP-AMP implements loader APIs to load subcore firmware in App Image Format to Internal RAM.

This section introduces how subcore firmware is loaded into DRAM by maincore firmware, depending on the where the subcore firmware is stored.


### Load from flash partition

For subcore firmware placed in flash partition, the following demo code can be used to load the subcore firmware from flash to internal RAM.

``` c
const esp_partition_t *sub_partition = esp_partition_find_first(PARTITION_TYPE, PARTITION_SUBTYPE, NULL);
ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
```

Note that `PARTITION_TYPE` and `PARTITION_SUBTYPE` are the type and subtype of the flash partition. Users are free to choose other types and subtypes. Just make sure the type and subtype are the same as the ones defined in `partitions.csv`.

### Load from embedded binary

For subcore firmware embedded into `.rodata` section of maincore firmware, the following demo code can be used to load the embedded binary into internal RAM. 

``` c
extern const uint8_t subcore_xxx_bin_start[] asm("_binary_${SUBCORE_APP_NAME}_bin_start");
extern const uint8_t subcore_xxx_bin_end[]   asm("_binary_${SUBCORE_APP_NAME}_bin_end");
ESP_ERROR_CHECK(esp_amp_load_sub(subcore_xxx_bin_start));
```

If you encounter issues such as `_binary_${SUBCORE_APP_NAME}_bin_start not found`, you can follow the checklists below to identify the causes.

1. Check if unified build is enabled. Embedding subcore firmware is only supported in unified build mode.
2. Check if the subcore firmware is generated. The generated subcore firmware can be found in `build/subcore/${SUBCORE_APP_NAME}.bin`.
3. Check if the symbol name of embedded binary is correct. Check if the symbol name in your maincore firmware is the same as the one in the assembly file `build/${SUBCORE_APP_NAME}.bin.S`. It should follow the naming convention `_binary_${SUBCORE_APP_NAME}_bin_start` and `_binary_${SUBCORE_APP_NAME}_bin_end`.
4. Check if the subcore firmware is embedded in to maincore firmware. The embedded firmware is placed under `.rodata.embedded` section. You can check the map file of your maincore firmware (e.g. `build/xxx.map`) to see if the symbol name appears in the section.

## Sdkconfig Options

* `CONFIG_ESP_AMP_ENABLED`: Enable ESP-AMP
* `CONFIG_ESP_AMP_SUBCORE_TYPE`: Subcore type. The proper subcore type is automatically chosen for different SoCs. For ESP32-C6, `CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE` is the only choice supported. For ESP32-P4, the only choice is `CONFIG_ESP_AMP_SUBCORE_TYPE_HP_CORE`.

## Application Examples 

* [separate_build](../examples/build_system/separate_build/): demonstrates how to build subcore firmware separately and download it into flash partition.
* [unified_build](../examples/build_system/unified_build): demonstrates how to build subcore firmware along with maincore firmware in a single step. Subcore firmware can be embedded into maincore firmware or downloaded into flash partition, depending on the sdkconfig option `SUBCORE_FIRMWARE_LOCATION`.
