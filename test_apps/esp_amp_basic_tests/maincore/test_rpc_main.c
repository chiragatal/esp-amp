#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"


#define RPC_MAIN_CORE_CLIENT 0x0000
#define RPC_MAIN_CORE_SERVER 0x0001
#define RPC_SUB_CORE_CLIENT  0x1000
#define RPC_SUB_CORE_SERVER  0x1001

TEST_CASE("RPC client init/deinit", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_deinit());

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_client_init(NULL, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_deinit());

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("RPC server init/deinit", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_deinit());

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, -1, -1));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_deinit());

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_server_init(NULL, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_deinit());

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("RPC client run/stop", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    /* run & stop an un-initialized server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_client_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_client_stop());

    /* run & stop an initialized server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_stop());

    /* run & stop a stopped server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_stop());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_stop());

    /* deinit */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_client_deinit());

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("RPC server run/stop", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    /* run & stop an un-initialized server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_server_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_FAILED, esp_amp_rpc_server_stop());

    /* run & stop an initialized server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_stop());

    /* run & stop a stopped server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_run());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_stop());
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_stop());

    /* deinit */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_deinit());

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

static esp_amp_rpc_status_t rpc_service_1(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    return ESP_AMP_RPC_STATUS_OK;
}

static esp_amp_rpc_status_t rpc_service_2(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    return ESP_AMP_RPC_STATUS_OK;
}

static esp_amp_rpc_status_t rpc_service_3(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    return ESP_AMP_RPC_STATUS_OK;
}

TEST_CASE("RPC server add service", "[esp_amp]")
{
    TEST_ASSERT(esp_amp_init() == 0);

    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    /* init server */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_init(rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048));

    /* function added to same service id will replace the old function */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_add_service(1, rpc_service_1));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_add_service(1, rpc_service_2));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_add_service(1, rpc_service_3));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_add_service(1, rpc_service_1));

    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_OK, esp_amp_rpc_server_deinit());

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}
