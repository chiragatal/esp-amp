#include <stdint.h>
#include "esp_attr.h"

#include "esp_amp_platform.h"
#include "esp_amp.h"


static int sw_intr_id0_handler(void *arg)
{
    printf("subcore trigger intr 0...\r\n");
    esp_amp_sw_intr_trigger(SW_INTR_ID_0);
    return 0;
}

static int sw_intr_id1_handler(void *arg)
{
    printf("subcore trigger intr 1...\r\n");
    esp_amp_sw_intr_trigger(SW_INTR_ID_1);
    return 0;
}

static int sw_intr_id2_handler(void *arg)
{
    printf("subcore trigger intr 2...\r\n");
    esp_amp_sw_intr_trigger(SW_INTR_ID_2);
    return 0;
}

static int sw_intr_id3_handler(void *arg)
{
    printf("subcore trigger intr 3...\r\n");
    esp_amp_sw_intr_trigger(SW_INTR_ID_3);
    return 0;
}

int main(void)
{
    printf("Hello!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_3, sw_intr_id3_handler, NULL) == 0);

    while (1);

    printf("Bye!!\r\n");
    return 0;
}
