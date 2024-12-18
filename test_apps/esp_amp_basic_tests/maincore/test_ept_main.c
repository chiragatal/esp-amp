#include <stdio.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp_platform.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"

TEST_CASE("main-core endpoint api test", "[esp_amp]")
{

    printf("Endpoint API Test Begin\n");
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);
    rpmsg_dev->ept_list = NULL;
    TEST_ASSERT_NULL(esp_amp_rpmsg_create_endpoint(rpmsg_dev, 1, NULL, NULL, NULL));

    /* endpoint create test */
    printf("endpoint create test\n");
    for (int i = 0; i < 32; i++) {
        esp_amp_rpmsg_ept_t* ept = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
        TEST_ASSERT_NOT_NULL(ept);
        TEST_ASSERT_EQUAL_HEX32(ept, esp_amp_rpmsg_create_endpoint(rpmsg_dev, i * 2, NULL, NULL, ept));
    }

    printf("endpoint repeat test\n");
    /* endpoint repeat test */
    for (int i = 31; i >= 0; i--) {
        esp_amp_rpmsg_ept_t* ept = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
        TEST_ASSERT_NOT_NULL(ept);
        TEST_ASSERT_NULL(esp_amp_rpmsg_create_endpoint(rpmsg_dev, i * 2, NULL, NULL, ept));
        free(ept);
    }

    /* endpoint rebind and delete test */
    printf("endpoint rebind and delete test\n");
    for (int i = 0; i < 32; i++) {
        esp_amp_rpmsg_ept_t* ept = esp_amp_rpmsg_search_endpoint(rpmsg_dev, i * 2);
        TEST_ASSERT_NOT_NULL(ept);
        TEST_ASSERT_EQUAL_HEX32(ept, esp_amp_rpmsg_rebind_endpoint(rpmsg_dev, i * 2, NULL, NULL));
        TEST_ASSERT_EQUAL_HEX32(ept, esp_amp_rpmsg_delete_endpoint(rpmsg_dev, i * 2));
        TEST_ASSERT_NULL(esp_amp_rpmsg_rebind_endpoint(rpmsg_dev, i * 2, NULL, NULL));
        free(ept);
    }

    /* endpoint re-create test */
    printf("endpoint re-create test\n");
    for (int i = 0; i < 32; i++) {
        esp_amp_rpmsg_ept_t* ept = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
        TEST_ASSERT_NOT_NULL(ept);
        TEST_ASSERT_EQUAL_HEX32(ept, esp_amp_rpmsg_create_endpoint(rpmsg_dev, i, NULL, NULL, ept));
    }

    /* delete and free all endpoint */
    printf("delete and free all endpoint\n");
    for (int i = 31; i >= 0; i--) {
        esp_amp_rpmsg_ept_t* ept = esp_amp_rpmsg_search_endpoint(rpmsg_dev, i);
        TEST_ASSERT_NOT_NULL(ept);
        TEST_ASSERT_EQUAL_HEX32(ept, esp_amp_rpmsg_delete_endpoint(rpmsg_dev, i));
        free(ept);
    }

    printf("Endpoint API Test Complete\n");
}
