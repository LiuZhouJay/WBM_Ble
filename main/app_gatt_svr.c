#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "app_gatt_svr.h"
#include "esp_log.h"
#include "host/ble_gatt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_nimble_hci.h"

#include "app_ota.h"
#include "app_ble.h"
#include "app_localdata.h"
#include "hello_world.h"
#include "app_ble_function.h"

extern unsigned char hello_world_bin[];
extern unsigned int hello_world_bin_len;


extern unsigned char hello_world_bin2[];
extern unsigned int hello_world_bin_len2;

static const char *TAG = "Test";

extern uint32_t Terminal_mac;

static const char *model_num = "44444";

uint16_t service1_char1_handle = 1;
uint16_t service2_char1_handle = 2;
uint16_t service2_char2_handle = 3;
uint16_t service2_char3_handle = 4;

extern  uint16_t conn_handle;
extern char *device_name;
QueueHandle_t ble_receive_queue = NULL;

extern QueueHandle_t turmass_send_cache_queue;

void Send_notify_ble(const void *buf);

static int
gatt_svr_cb1(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int 
gatt_svr_cb2(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

//接收网关发送来的信息
void ble_receive_data(const char *data) {
    if (data != NULL) {
        Send_notify_ble(data);
    } else {
        ESP_LOGI(TAG, "data pointer is NULL");
    }
}

void write_queue(const char *data, uint16_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (pdPASS != xQueueSend(turmass_send_cache_queue, &data[i], portMAX_DELAY))
        {
            break;
        }
    }

}

void Send_notify_ble(const void *buf){
    struct os_mbuf *txom;
    txom = ble_hs_mbuf_from_flat(buf, strlen(buf));
    int rc= ble_gatts_notify_custom(conn_handle,service1_char1_handle,txom);
    if (rc == 0) {
        MODLOG_DFLT(INFO, "Notification sent successfully");
    } else {
        MODLOG_DFLT(INFO, "Error in sending notification rc = %d", rc);
    }
}

uint32_t decimalToHex(uint32_t decimal) {
    // 我们需要8个十六进制位来表示32位的uint32_t
    uint32_t hexDigits[8];
    int index = 0;

    // 处理每一位，直到数值为0
    while (decimal > 0) {
        uint32_t hexDigit = decimal % 16; // 获取当前的十六进制位
        // 将十进制的数值转换为字符
        char hexChar = (hexDigit < 10) ? (hexDigit + '0') : (hexDigit - 10 + 'A');
        // 存储十六进制字符
        hexDigits[index++] = hexChar;
        // 移除已经处理的十六进制位
        decimal /= 16;
    }

    // 由于我们是从低位开始处理的，所以需要反转数组
    uint32_t hexValue = 0;
    for (int i = index - 1; i >= 0; --i) {
        hexValue = (hexValue << 4) | (hexDigits[i] - (hexDigits[i] >= 'A' ? 55 : 48));
    }
    ESP_LOGI(TAG, "value213: %lu", hexValue);
    return hexValue;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* ******************Service1***************************
        ********该服务主要用来读取特定消息和接收蓝牙通知************
        */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        /*BLE_UUID16_DECLARE 是 ESP-IDF 中用于声明 16 位 UUID 的宏。
        它用于将 16 位的 UUID 值转换为适合在代码中使用的格式*/
        .uuid = BLE_UUID16_DECLARE(GATT_SEVER_1_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Characteristic1*/
                .uuid = BLE_UUID16_DECLARE(GATT_SEVER_1_CHARACTERISTIC_1_UUID),
                .access_cb = gatt_svr_cb1,
                .val_handle = &service1_char1_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            }, {
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        /* ******************Service2***************************
        ********该服务主要用来改变终端的mac地址和终端名称以及向终端发送消息***********
        */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_SEVER_2_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* 特征值1 ***** 主要用来向网关发送消息***********/
                .uuid = BLE_UUID16_DECLARE(GATT_SEVER_2_CHARACTERISTIC_1_UUID),
                .access_cb = gatt_svr_cb2,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle=&service2_char1_handle,
            },
            {
                /* 特征值1 ***** 更改设备名称***********/
                .uuid = BLE_UUID16_DECLARE(GATT_SEVER_2_CHARACTERISTIC_2_UUID),
                .access_cb = gatt_svr_cb2,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle=&service2_char2_handle,
            },
            {
                /* 特征值1 ***** 更改设备mac地址***********/
                .uuid = BLE_UUID16_DECLARE(GATT_SEVER_2_CHARACTERISTIC_3_UUID),
                .access_cb = gatt_svr_cb2,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle=&service2_char3_handle,
            },
            {
                /* 特征值1 ***** 更改设备mac地址***********/
                .uuid = BLE_UUID16_DECLARE(GATT_SEVER_2_CHARACTERISTIC_4_UUID),
                .access_cb = gatt_svr_cb2,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle=&service2_char3_handle,
            }, {
                0, /* No more characteristics in this service */
            },
        }
        
    },

    {
        0, /* No more services */
    },
};

/*特征值回调函数
*如果只有一个特征值，可以通过判断是读事件还是写事件去分别操作
*如果有两个特征值，可以先判断特征值UUID，在判断事件

BLE_GATT_ACCESS_OP_READ_CHR:
这个操作类型表示对 GATT 特征值（Characteristic）进行读取操作。
在 BLE 中，GATT 特征值是设备之间交换数据的基本单元。每个特征值都有一个唯一的句柄（handle），并包含了关联的属性，如读、写、通知等。
当一个设备想要读取另一个设备的特征值时，就会使用 BLE_GATT_ACCESS_OP_READ_CHR 操作类型。

BLE_GATT_ACCESS_OP_READ_DSC:
这个操作类型表示对 GATT 描述符（Descriptor）进行读取操作。
描述符是与特征值相关联的附加信息单元，用于提供关于特征值的额外信息，例如格式、单位等。
当一个设备想要读取特征值的描述符时，就会使用 BLE_GATT_ACCESS_OP_READ_DSC 操作类型。*/

static int
gatt_svr_cb1(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    //判断是读事件还是写事件
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR){
        ESP_LOGI(TAG, "Sever1 ch1 Doing RADE");
        os_mbuf_append(ctxt->om, (const void*)model_num, strlen(model_num));
    }
    return 0;
}

static int gatt_svr_cb2(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    uuid = ble_uuid_u16(ctxt->chr->uuid);
    //判断特征值UUID，再判断事件
    if (uuid == GATT_SEVER_2_CHARACTERISTIC_1_UUID) {
        //判断是读事件还是写事件
        const uint8_t* data = ctxt->om->om_data;
        uint16_t data_len = ctxt->om->om_len;

        // 分配一个新的缓冲区来存储转换后的字符串
        // 这里假设每个字符占用2个字节
        char* str_buf = (char*)malloc(data_len + 1);
        if (!str_buf) {
            ESP_LOGE(TAG, "Memory allocation failed");
        }

        // 将UTF-16小端编码的数据转换为C字符串
        for (uint16_t i = 0; i < data_len; i += 2) {
            uint16_t char_code = data[i] | (data[i + 1] << 8); // 组合两个字节为一个16位的Unicode字符
            // 确保字符码不超过ASCII范围
            if (char_code <= 0x7F) {
                str_buf[i / 2] = (char)char_code;
            } else {
                // 遇到非法字符，替换为问号
                str_buf[i / 2] = '?';
            }
        }
        str_buf[data_len / 2] = '\0'; // 添加字符串终止符
        // 打印转换后的字符串
        ESP_LOGI(TAG, "Received string: %s", str_buf);
        write_queue(str_buf,ctxt->om->om_len);
        free(str_buf);     
            // ota_init();
    }
    if (uuid == GATT_SEVER_2_CHARACTERISTIC_2_UUID) {
        const uint8_t* data = ctxt->om->om_data;
        uint16_t data_len = ctxt->om->om_len;

        // 分配一个新的缓冲区来存储转换后的字符串
        // 这里假设每个字符占用2个字节
        char* str_buf = (char*)malloc(data_len + 1);
        if (!str_buf) {
            ESP_LOGE(TAG, "Memory allocation failed");
        }

        // 将UTF-16小端编码的数据转换为C字符串
        for (uint16_t i = 0; i < data_len; i += 2) {
            uint16_t char_code = data[i] | (data[i + 1] << 8); // 组合两个字节为一个16位的Unicode字符
            // 确保字符码不超过ASCII范围
            if (char_code <= 0x7F) {
                str_buf[i / 2] = (char)char_code;
            } else {
                // 遇到非法字符，替换为问号
                str_buf[i / 2] = '?';
            }
        }
        str_buf[data_len / 2] = '\0'; // 添加字符串终止符

        // 打印转换后的字符串
        ESP_LOGI(TAG, "Received string: %s", str_buf);
        modification_name(str_buf); 
        // 释放之前分配的字符串缓冲区
        free(str_buf);     
    }

    if (uuid == GATT_SEVER_2_CHARACTERISTIC_3_UUID) {
        // 假设 om_data 是接收到的 ArrayBuffer // 读取 Uint32 值，false 表示使用小端字节   
        // 假设 om_data 是接收到的 ArrayBuffer
        // 并且 om_len 是接收到的数据长度
       if (ctxt->om->om_len == 4) { // 确保接收到的数据长度为4字节
            uint32_t value;
            const uint8_t* data = ctxt->om->om_data;
            // 根据字节序解析Uint32值
            // 这里假设小端序，如果大端序需要调整
            value = (uint32_t)data[0] | (uint32_t)(data[1] << 8) | (uint32_t)(data[2] << 16) | (uint32_t)(data[3] << 24);
            ESP_LOGI(TAG, "Received value: %lu", value);
            decimalToHex(value);
        }
    }

    if (uuid == GATT_SEVER_2_CHARACTERISTIC_4_UUID) {
        if (ctxt->om->om_len == 2) { // 确保接收到的数据长度为2字节
            uint16_t value;
            const uint8_t* data = ctxt->om->om_data;
            // 根据字节序解析Uint16值
            // 这里假设小端序，如果大端序需要调整
            value = (uint16_t)data[0] | (uint16_t)(data[1] << 8);
            ESP_LOGI(TAG, "Received value: %d", value);
            ble_function_select(value);
        }
    }
    return 0;
}
int
gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    //该函数用于计算服务和特征值的数量，以便后续调用ble_gatts_add_svcs()函数动态注册服务和特征值
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
    //该函数用于动态注册服务和特征值
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
    /*使用上述两个 API 注册 GATT 服务时必须保证协议栈还没有同步，即 Host 协议栈还没有启动；
    上述两个 API 使用之后并没有将 GATT 服务真正的注册进 Host 中，只更新了一些全局变量用来记录自定义的 GATT 服务，
    真正的 GATT 服务注册会在 Host 协议栈真正启动时调用的 ble_gatts_start() 函数中注册。*/

     //xTaskCreate(send_notification, "Send_notification", 2048, NULL, 2, NULL);

    //  ble_receive_queue = xQueueCreate(1024 * 3, 1);
    // xTaskCreate(ble_receive, "ble_receive_data", 2048, NULL, 2, NULL);
    

    return 0;
}