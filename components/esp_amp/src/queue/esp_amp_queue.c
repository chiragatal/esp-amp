/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp_priv.h"
#include "esp_amp_queue.h"
#include "esp_attr.h"

int IRAM_ATTR esp_amp_queue_send_try(esp_amp_queue_t *queue, void* data, uint16_t size)
{
    if (!queue->master) {
        // can only be called on `master-core`
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (queue->used_index == queue->free_index) {
        // send before alloc!
        return ESP_ERR_NOT_ALLOWED;
    }
    if (queue->max_item_size < size) {
        // exceeds max size
        return ESP_ERR_NO_MEM;
    }

    uint16_t q_idx = queue->used_index & (queue->size - 1);
    uint16_t flags = queue->desc[q_idx].flags;
    RISCV_MEMORY_BARRIER();
    if (!ESP_AMP_QUEUE_FLAG_IS_USED(queue->used_flip_counter, flags)) {
        // no free buffer slot to use, send fail, this should not happen
        return ESP_ERR_NOT_ALLOWED;
    }

    queue->desc[q_idx].addr = (uint32_t)(data);
    queue->desc[q_idx].len = size;
    RISCV_MEMORY_BARRIER();
    // make sure the buffer address and size are set before making the slot available to use
    queue->used_index += 1;
    queue->desc[q_idx].flags ^= ESP_AMP_QUEUE_AVAILABLE_MASK(1);
    /*
        Since we confirm that ESP_AMP_QUEUE_FLAG_IS_USED is true, so at this moment, AVAILABLE flag should be different from the flip_counter.
        To set the AVAILABLE flag the same as the flip_counter, we just XOR the corresponding bit with 1, which will make it equal to the flip_counter.
    */
    if (q_idx == queue->size - 1) {
        // update the filp_counter if necessary
        queue->used_flip_counter = !queue->used_flip_counter;
    }

    // notify the opposite side if necessary
    if (queue->notify_fc != NULL) {
        return queue->notify_fc(queue->priv_data);
    }

    return ESP_OK;
}

int IRAM_ATTR esp_amp_queue_recv_try(esp_amp_queue_t *queue, void** buffer, uint16_t* size)
{
    *buffer = NULL;
    *size = 0;
    if (queue->master) {
        // can only be called on `remote-core`
        return ESP_ERR_NOT_SUPPORTED;
    }

    uint16_t q_idx = queue->free_index & (queue->size - 1);
    uint16_t flags = queue->desc[q_idx].flags;
    RISCV_MEMORY_BARRIER();

    if (!ESP_AMP_QUEUE_FLAG_IS_AVAILABLE(queue->free_flip_counter, flags)) {
        // no available buffer slot to receive, receive fail
        return ESP_ERR_NOT_FOUND;
    }

    *buffer = (void*)(queue->desc[q_idx].addr);
    *size = queue->desc[q_idx].len;
    // make sure the buffer address and size are read and saved before returning
    queue->free_index += 1;

    if (q_idx == queue->size - 1) {
        // update the filp_counter if necessary
        queue->free_flip_counter = !queue->free_flip_counter;
    }

    return ESP_OK;
}


int IRAM_ATTR esp_amp_queue_alloc_try(esp_amp_queue_t *queue, void** buffer, uint16_t size)
{
    *buffer = NULL;
    if (!queue->master) {
        // can only be called on `master-core`
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (queue->max_item_size < size) {
        // exceeds max size
        return ESP_ERR_NO_MEM;
    }

    uint16_t q_idx = queue->free_index & (queue->size - 1);
    uint16_t flags = queue->desc[q_idx].flags;
    RISCV_MEMORY_BARRIER();
    if (!ESP_AMP_QUEUE_FLAG_IS_USED(queue->free_flip_counter, flags)) {
        // no available buffer slot to alloc, alloc fail
        return ESP_ERR_NOT_FOUND;
    }

    *buffer = (void*)(queue->desc[q_idx].addr);
    queue->free_index += 1;

    if (q_idx == queue->size - 1) {
        // update the filp_counter if necessary
        queue->free_flip_counter = !queue->free_flip_counter;
    }

    return ESP_OK;
}

int IRAM_ATTR esp_amp_queue_free_try(esp_amp_queue_t *queue, void* buffer)
{
    if (queue->master) {
        // can only be called on `remote-core`
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (queue->used_index == queue->free_index) {
        // free before receive!
        return ESP_ERR_NOT_ALLOWED;
    }

    uint16_t q_idx = queue->used_index & (queue->size - 1);
    uint16_t flags = queue->desc[q_idx].flags;
    RISCV_MEMORY_BARRIER();
    if (!ESP_AMP_QUEUE_FLAG_IS_AVAILABLE(queue->used_flip_counter, flags)) {
        // no available buffer slot to place freed buffer, free fail, this should not happen
        return ESP_ERR_NOT_ALLOWED;
    }

    queue->desc[q_idx].addr = (uint32_t)(buffer);
    queue->desc[q_idx].len = queue->max_item_size;
    RISCV_MEMORY_BARRIER();
    // make sure the buffer address and size are set before making the slot available to use
    queue->used_index += 1;
    queue->desc[q_idx].flags ^= ESP_AMP_QUEUE_USED_MASK(1);
    /*
        Since we confirm that ESP_AMP_QUEUE_FLAG_IS_AVAILABLE is true, so at this moment, USED flag should be different from the flip_counter.
        To set the USED flag the same as the flip_counter, we just XOR the corresponding bit with 1, which will make it equal to the flip_counter.
    */
    if (q_idx == queue->size - 1) {
        // update the filp_counter if necessary
        queue->used_flip_counter = !queue->used_flip_counter;
    }

    return ESP_OK;
}

int esp_amp_queue_init_buffer(esp_amp_queue_conf_t* queue_conf, uint16_t queue_len, uint16_t queue_item_size, esp_amp_queue_desc_t* queue_desc, void* queue_buffer)
{
    queue_conf->queue_size = queue_len;
    queue_conf->max_queue_item_size = queue_item_size;
    queue_conf->queue_desc = queue_desc;
    queue_conf->queue_buffer = queue_buffer;
    uint8_t* _queue_buffer = (uint8_t*)queue_buffer;
    for (uint16_t desc_idx = 0; desc_idx < queue_conf->queue_size; desc_idx++) {
        queue_conf->queue_desc[desc_idx].addr = (uint32_t)_queue_buffer;
        queue_conf->queue_desc[desc_idx].flags = 0;
        queue_conf->queue_desc[desc_idx].len = queue_item_size;
        _queue_buffer += queue_item_size;
    }
    return ESP_OK;
}

int esp_amp_queue_create(esp_amp_queue_t* queue, esp_amp_queue_conf_t* queue_conf, esp_amp_queue_cb_t cb_func, esp_amp_queue_cb_t ntf_func, void* priv_data, bool is_master)
{
    queue->size = queue_conf->queue_size;
    queue->desc = queue_conf->queue_desc;
    queue->free_flip_counter = 1;
    queue->used_flip_counter = 1;
    queue->free_index = 0;
    queue->used_index = 0;
    queue->max_item_size = queue_conf->max_queue_item_size;
    queue->callback_fc = cb_func;
    queue->notify_fc = ntf_func;
    queue->priv_data = priv_data;
    queue->master = is_master;
    return ESP_OK;
}

