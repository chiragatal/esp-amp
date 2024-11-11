# ESP-AMP Peripheral Guide

This document covers the details of peripheral development in subcore applications. Guidelines to ensure the proper operation of peripherals are also provided.

## Overview

ESP SoCs have two types of peripherals: HP peripherals and LP peripherals. As their name indicates, HP peripherals are dedicated to HP system and mainly operated by HP core, while LP peripherals can keep active when HP core is in sleep mode, with or without the involvement of LP core. 

ESP-AMP allows users to control both HP and LP peripherals in both maincore and subcore applications. However, simultaneous access to a same peripheral by both maincore and subcore can lead to unpredictable results.

## Usage

ESP-IDF has implemented a complete set of HP peripheral drivers dedicated for FreeRTOS environment. These drivers are found under `components/esp_driver_xxx` and can be easily employed for maincore application development. For more details, please refer to ESP-IDF [peripherals API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/index.html). However, subcore applications running bare-metal, Zephyr or other RTOS cannot benefit from these drivers. In thses cases, low-level hal drivers (`<peripheral>_ll.h`) under `components/hal/<target>/include/hal` can be used as an alternative.

Supported peripherals in different types of subcore is listed below:

| Peripheral | Subcore Type | Capability |
| :--- | :--- | :--- |
| HP | HP | ll_drivers can be used. Interrupt handling is supported |
| HP | LP | ll_drivers can be used. Interrupt handling is not supported at present |
| LP | HP | not supported |
| LP | LP | ULP component can be used. Interrupt handling is supported |

The following sections elaborate on the usage and guidelines of peripherals in these scenarios.

### Use HP Peripherals in HP/LP Subcore

HP peripherals can be accessed by both HP core and LP core. Although IDF's driver component cannot be used by subcore application running bare-metal environment, ll_drivers is a nice alternative to control HP peripherals among different SoCs in a consistent way.

Low level hal drivers can be found in ESP-IDF hal component. It is designed to be OS-agnostic, and can be used in bare-metal, Zephyr and nuttx environment. APIs in hal component are designed to be consistent among different SoCs.

Although ESP-AMP does not prohibit simultaneous access to a same peripheral by both maincore and subcore, this operation can lead to unpredictable results. For example, `driver_init()` APIs in ESP-IDF always reset the control registers of peripherals. If this peripheral is being used by subcore, and `driver_init()` is called by maincore, subcore will get unexpected input or output without being aware of it. The guideline to avoid this issue is, make sure that the same HP peripheral is accessed by only one core within a single operational period (from `driver_init()` to `driver_deinit()`) of the peripheral.

### Use LP Peripherals in LP Subcore

For ESP-AMP projects with LP core as subcore, it is recommended for LP core to operate LP peripherals only. These periphrals belongs to RTC system, with different power and clk domain from HP system. LP core can fully control these periphrals, including interrupt registration and handling, without the need to worry about the impact of HP core.

Supported LP peripherals in C6 and P4 are listed below:

| Peripheral | ESP32-C6 | ESP32-P4 |
| :--- | :--- | :--- |
| LP IO | Yes | Yes |
| LP I2C | Yes | Yes |
| LP UART | Yes | Yes |
| LP SPI | No | Yes |

ESP-IDF's ULP component has implemented LP peripheral drivers. LP subcore developers can benefit from that to speed up the firmware implementation. For more details, please refer to ESP-IDF API reference of [C6 ulp lp core](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/system/ulp-lp-core.html) for C6 or [P4 ulp lp core](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-reference/system/ulp-lp-core.html) for P4.
