#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "stdlib.h"
#include "hello_world.h"

static const char *TAG = "OTA Update";

extern unsigned char hello_world_bin[];
extern unsigned int hello_world_bin_len;


// extern unsigned char hello_world_bin2[];
// extern unsigned int hello_world_bin_len2;

// extern unsigned char Nimble_bin[];
// extern unsigned int Nimble_bin_len;

// 定义每个块的大小，确保它适合您的内存和分区大小
#define BLOCK_SIZE (1024 * 16) // 例如，16KB

void ota_upgrade(void)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    size_t bytes_written = 0; // 已写入的字节数

    ESP_LOGI(TAG, "Starting OTA example task");

    // 获取下一个可用的OTA更新分区
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Could not find a valid OTA update partition");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lu",
             update_partition->subtype, update_partition->address);

    // 准备开始OTA更新
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    // 计算需要写入的总块数
    size_t total_blocks = hello_world_bin_len/ BLOCK_SIZE;
    if (hello_world_bin_len % BLOCK_SIZE != 0) {
        total_blocks++; // 如果有剩余的字节，也需要一个额外的块
    }

    // 分块写入固件
    for (size_t block = 0; block < total_blocks; block++) {
        size_t block_offset = block * BLOCK_SIZE;
        size_t block_data_size = (block == total_blocks - 1) ? (hello_world_bin_len - block_offset) : BLOCK_SIZE;

        err = esp_ota_write(update_handle, hello_world_bin + block_offset, block_data_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            esp_ota_abort(update_handle);
            vTaskDelete(NULL);
        }
        ESP_LOGI(TAG, "Written block %d of %d, size %zu bytes", block + 1, total_blocks, block_data_size);
        bytes_written += block_data_size;
    }

    // 结束OTA更新
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        vTaskDelete(NULL);
    } else {
        ESP_LOGI(TAG, "esp_ota_end succeeded");
        // 设置新的OTA分区为启动分区
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
            vTaskDelete(NULL);
        } else {
            ESP_LOGI(TAG, "Prepare to restart system!");
            // 重启系统以应用新的固件
            esp_restart();
        }
    }
}


void ota_init(void)
{
    ESP_LOGI(TAG, "OTA example app_main start");
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    if (running_partition) {
        ESP_LOGI(TAG, "Running partition label: %s", running_partition->label);
        // 其他操作...
    } else {
        ESP_LOGE(TAG, "Failed to get running partition");
    }
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition) {
        ESP_LOGI(TAG, "next partition label: %s", update_partition->label);
        // 其他操作...
    } else {
        ESP_LOGE(TAG, "Could not find a valid OTA update partition");
    }
    // 创建OTA示例任务

}