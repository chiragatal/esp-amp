cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")

if(DEFINED ENV{ESP_AMP_PATH})
  set(ESP_AMP_PATH $ENV{ESP_AMP_PATH})
else()
  set(ESP_AMP_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../..)
endif(DEFINED ENV{ESP_AMP_PATH})

include(${ESP_AMP_PATH}/components/esp_amp/cmake/subcore_project.cmake)

# SUBCORE_APP_NAME is defined in subcore_config.cmake
set(PROJECT_VER "1.0")
project(subcore_separate_build)
