
//第二版，两个服务

/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_svr.h"
#include "esp_log.h"
#include "host/ble_gatt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "ota.h"

#include "app_rs485.h"
#include "app_turmass.h"

#include "esp_nimble_hci.h"

static const char *TAG = "Test";

extern uint32_t Terminal_mac;

static const char *model_num = "2222";

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
    static char data[256];
    uint16_t uuid;
    uuid = ble_uuid_u16(ctxt->chr->uuid);
    //判断特征值UUID，再判断事件
    if (uuid == GATT_SEVER_2_CHARACTERISTIC_1_UUID) {
        //判断是读事件还是写事件
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR){

            if(ctxt->om->om_len <= sizeof(data)){
                strncpy(data,(char *)(ctxt->om->om_data),ctxt->om->om_len);
                data[ctxt->om->om_len] = 0;
                ESP_LOGI(TAG, "data: %s",data);
                write_queue(data,ctxt->om->om_len);
            }else{
                char *value = (char *)malloc(ctxt->om->om_len);
                strncpy(value,(char *)(ctxt->om->om_data),ctxt->om->om_len);
                value[ctxt->om->om_len] = 0;
                ESP_LOGI(TAG, "value: %s",value);
                write_queue(value,ctxt->om->om_len);
                free(value);
            }
        }
    }
    if (uuid == GATT_SEVER_2_CHARACTERISTIC_2_UUID) {
        //strncpy(device_name,(char *)(ctxt->om->om_data),ctxt->om->om_len);
        ota_init();
        ESP_LOGI(TAG, "Sever2 ch2 Received date: %s",device_name);
        
    }

    if (uuid == GATT_SEVER_2_CHARACTERISTIC_3_UUID) {
        ota_upgrade();
    //    Terminal_mac = (uint32_t)*ctxt->om->om_data ;
        ESP_LOGI(TAG, "Terminal_mac: %lu",Terminal_mac);
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