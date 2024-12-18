#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"

extern const uint8_t subcore_rpmsg_bin_start[] asm("_binary_subcore_test_rpmsg_bin_start");
extern const uint8_t subcore_rpmsg_bin_end[]   asm("_binary_subcore_test_rpmsg_bin_end");

esp_amp_rpmsg_dev_t subcore_rpmsg_dev;
typedef struct rpmsg_test_pars_t {
    esp_amp_rpmsg_dev_t* rpmsg_dev;
    QueueHandle_t q_handler;
    SemaphoreHandle_t sem_handler;
    esp_amp_rpmsg_ept_t* rpmsg_ept;
} rpmsg_test_pars_t;

static void subcore_rpmsg_test_subcore_init(void)
{
    /* Load subcore firmware */
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_load_sub(subcore_rpmsg_bin_start));

    /* Run subcore */
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_start_subcore());
}

int IRAM_ATTR rpmsg_test_ept_isr_ctx(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    // zero copy
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rpmsg_test_pars_t* pars = (rpmsg_test_pars_t*)rx_cb_data;
    if (xQueueSendFromISR(pars->q_handler, &msg_data, &xHigherPriorityTaskWoken) != pdTRUE) {
        esp_amp_rpmsg_destroy(pars->rpmsg_dev, msg_data);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    return 0;
}

void ept0_task_ctx(void* arg)
{
    rpmsg_test_pars_t* pars = (rpmsg_test_pars_t*)arg;    void* data;
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(pars->q_handler, &data, portMAX_DELAY));
    printf("EPT0: Message from Sub core ==> %s\n", (char*)(data));
    TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_destroy(pars->rpmsg_dev, data));

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(pars->sem_handler));
    vTaskDelete(NULL);
}

void ept1_task_ctx(void* arg)
{
    rpmsg_test_pars_t* pars = (rpmsg_test_pars_t*)arg;    void* data;
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(pars->q_handler, &data, portMAX_DELAY));
    printf("EPT1: Message from Sub core ==> %s\n", (char*)(data));
    TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_destroy(pars->rpmsg_dev, data));
    data = esp_amp_rpmsg_create_message(pars->rpmsg_dev, 32, ESP_AMP_RPMSG_DATA_DEFAULT);
    TEST_ASSERT_NOT_NULL(data);
    snprintf(data, 30, "Rsp from EPT1: 1");
    TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_send_nocopy(pars->rpmsg_dev, pars->rpmsg_ept, 0, data, 30));

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(pars->sem_handler));
    vTaskDelete(NULL);
}

void ept2_task_ctx(void* arg)
{
    rpmsg_test_pars_t* pars = (rpmsg_test_pars_t*)arg;
    int algo_counter = 0;
    uint16_t dst_addr = 0;
    int* result = NULL;

    for (int i = 0; i < 2; i++) {
        int a = rand() % 1000;
        int b = rand() % 1000;
        int c = 0;
        if (algo_counter) {
            c = a + b;
            dst_addr = 1;
            printf("Generating %d + %d = %d\n", a, b, c);
        } else {
            c = a * b;
            dst_addr = 2;
            printf("Generating %d x %d = %d\n", a, b, c);
        }

        algo_counter = !algo_counter;
        int* data = (int*)(esp_amp_rpmsg_create_message(pars->rpmsg_dev, sizeof(int) * 2, ESP_AMP_RPMSG_DATA_DEFAULT));
        TEST_ASSERT_NOT_NULL(data);
        data[0] = a;
        data[1] = b;
        TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_send_nocopy(pars->rpmsg_dev, pars->rpmsg_ept, dst_addr, data, sizeof(int) * 2));
        printf("Sending requests successfully. Waiting for response\n");
        TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(pars->q_handler, &result, portMAX_DELAY));
        TEST_ASSERT_EQUAL(c, *result);
        TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_destroy(pars->rpmsg_dev, result));
    }

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(pars->sem_handler));
    vTaskDelete(NULL);
}

TEST_CASE("rpmsg transport layer test between main-core and sub-core", "[esp_amp]")
{

    /* init esp amp component */
    TEST_ASSERT_EQUAL_INT(0, esp_amp_init());

    /* init transport layer */
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);
    TEST_ASSERT_EQUAL(0, esp_amp_rpmsg_main_init(rpmsg_dev, 16, 64, false, false));

    /* create FreeRTOS Queue */
    QueueHandle_t ept0_msg_q = xQueueCreate(32, sizeof(void*));
    QueueHandle_t ept1_msg_q = xQueueCreate(32, sizeof(void*));
    QueueHandle_t ept2_msg_q = xQueueCreate(32, sizeof(void*));
    TEST_ASSERT_NOT_NULL(ept0_msg_q);
    TEST_ASSERT_NOT_NULL(ept1_msg_q);
    TEST_ASSERT_NOT_NULL(ept2_msg_q);

    /* create FreeRTOS semaphore */
    SemaphoreHandle_t ept0_sem = xSemaphoreCreateBinary();
    SemaphoreHandle_t ept1_sem = xSemaphoreCreateBinary();
    SemaphoreHandle_t ept2_sem = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_NULL(ept0_sem);
    TEST_ASSERT_NOT_NULL(ept1_sem);
    TEST_ASSERT_NOT_NULL(ept2_sem);

    /* allocate endpoint */
    esp_amp_rpmsg_ept_t* ept0 = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
    esp_amp_rpmsg_ept_t* ept1 = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
    esp_amp_rpmsg_ept_t* ept2 = (esp_amp_rpmsg_ept_t*)(malloc(sizeof(esp_amp_rpmsg_ept_t)));
    TEST_ASSERT_NOT_NULL(ept0);
    TEST_ASSERT_NOT_NULL(ept1);
    TEST_ASSERT_NOT_NULL(ept2);

    /* create callback params */
    rpmsg_test_pars_t pars0 = {
        .q_handler = ept0_msg_q,
        .sem_handler = ept0_sem,
        .rpmsg_dev = rpmsg_dev,
        .rpmsg_ept = ept0
    };
    rpmsg_test_pars_t pars1 = {
        .q_handler = ept1_msg_q,
        .sem_handler = ept1_sem,
        .rpmsg_dev = rpmsg_dev,
        .rpmsg_ept = ept1
    };
    rpmsg_test_pars_t pars2 = {
        .q_handler = ept2_msg_q,
        .sem_handler = ept2_sem,
        .rpmsg_dev = rpmsg_dev,
        .rpmsg_ept = ept2
    };

    /* create endpoint and attach callback */
    TEST_ASSERT_EQUAL_HEX32(pars0.rpmsg_ept, esp_amp_rpmsg_create_endpoint(pars0.rpmsg_dev, 0, rpmsg_test_ept_isr_ctx, (void*)&pars0, pars0.rpmsg_ept));
    TEST_ASSERT_EQUAL_HEX32(pars1.rpmsg_ept, esp_amp_rpmsg_create_endpoint(pars1.rpmsg_dev, 1, rpmsg_test_ept_isr_ctx, (void*)&pars1, pars1.rpmsg_ept));
    TEST_ASSERT_EQUAL_HEX32(pars2.rpmsg_ept, esp_amp_rpmsg_create_endpoint(pars2.rpmsg_dev, 2, rpmsg_test_ept_isr_ctx, (void*)&pars2, pars2.rpmsg_ept));
    TEST_ASSERT_EQUAL_INT(0, esp_amp_rpmsg_intr_enable(rpmsg_dev));

    TEST_ASSERT_NOT_EQUAL(pdFAIL, xTaskCreate(ept0_task_ctx, "ept0", 2048, (void*)&pars0, tskIDLE_PRIORITY, NULL));
    TEST_ASSERT_NOT_EQUAL(pdFAIL, xTaskCreate(ept1_task_ctx, "ept1", 2048, (void*)&pars1, tskIDLE_PRIORITY, NULL));
    TEST_ASSERT_NOT_EQUAL(pdFAIL, xTaskCreate(ept2_task_ctx, "ept2", 2048, (void*)&pars2, tskIDLE_PRIORITY, NULL));

    /* load subcore and start remote processor */
    subcore_rpmsg_test_subcore_init();

    /* sync all tasks */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(pars0.sem_handler, portMAX_DELAY));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(pars1.sem_handler, portMAX_DELAY));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(pars2.sem_handler, portMAX_DELAY));

    /* free heap */
    vQueueDelete(pars0.q_handler);
    vQueueDelete(pars1.q_handler);
    vQueueDelete(pars1.q_handler);
    vSemaphoreDelete(pars0.sem_handler);
    vSemaphoreDelete(pars1.sem_handler);
    vSemaphoreDelete(pars2.sem_handler);
    free(rpmsg_dev);
    free(pars0.rpmsg_ept);
    free(pars1.rpmsg_ept);
    free(pars2.rpmsg_ept);

    /* wait for idle task to recycle task stack */
    vTaskDelay(pdMS_TO_TICKS(1000));
}
