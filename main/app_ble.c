#include "esp_log.h" // 包含ESP-IDF的日志库，用于输出日志信息
#include "nvs_flash.h" // 包含NVS（非易失性存储器）的头文件，用于存储和读取配置数据
#include "freertos/FreeRTOSConfig.h" // 包含FreeRTOS的配置头文件

/* BLE相关头文件 */
#include "nimble/nimble_port.h" // NimBLE端口层的头文件
#include "nimble/nimble_port_freertos.h" // NimBLE在FreeRTOS上的移植层头文件
#include "host/ble_hs.h" // NimBLE主机层的头文件
#include "host/util/util.h" // NimBLE的实用工具库
#include "console/console.h" // NimBLE控制台接口
#include "services/gap/ble_svc_gap.h" // NimBLE的通用访问配置文件（GAP）服务
#include "app_gatt_svr.h" // NimBLE的通用属性配置文件（GATT）服务

#include "app_localdata.h"

// 定义日志标签
static const char *tag = "NimBLE_BLE_HeartRate";

// 定义连接句柄，用于BLE连接管理
uint16_t conn_handle;

// // 定义设备名称，用于BLE设备的广告
char device_name[32] = "wBms_Terminal2";
size_t length = sizeof(device_name);

// BLE GAP事件处理函数声明
static int blehr_gap_event(struct ble_gap_event *event, void *arg);

// 定义BLE地址类型
static uint8_t blehr_addr_type;

void modification_name(const char* value);

void
print_addr(const void *addr)
{
    const uint8_t *u8p;

    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

// 广告函数，用于设置BLE设备的广告参数和开始广播
static void blehr_advertise(void)
{
    // 定义广告参数结构体和广告字段结构体
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    // 初始化广告字段结构体
    memset(&fields, 0, sizeof(fields));

    // 设置广告数据，包括标志、TX功率和设备名称
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    // 设置广告字段
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    // 开始广播
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(blehr_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, blehr_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

// BLE GAP事件处理函数，处理BLE连接、断开、广播完成等事件
static int blehr_gap_event(struct ble_gap_event *event, void *arg)
{
    // 根据事件类型进行处理
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            // 处理连接事件，记录连接状态和句柄
            MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status != 0) {
                // 连接失败，重新开始广播
                blehr_advertise();
            }
            conn_handle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            // 处理断开连接事件，记录原因
            MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
            // 断开连接，重新开始广播
            blehr_advertise();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            // 处理广播完成事件
            MODLOG_DFLT(INFO, "adv complete\n");
            blehr_advertise();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            // 处理订阅事件
            ESP_LOGI("BLE_GAP_SUBSCRIBE_EVENT", "conn_handle from subscribe=%d", conn_handle);
            break;

        case BLE_GAP_EVENT_MTU:
            // 处理MTU（最大传输单元）更新事件
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.value);
            break;

        // 其他事件类型可以继续添加处理
    }

    return 0;
}

// BLE同步事件处理函数，用于在BLE主机初始化后执行的操作
static void blehr_on_sync(void)
{
    int rc;
    // 自动推断BLE地址类型
    rc = ble_hs_id_infer_auto(0, &blehr_addr_type);
    assert(rc == 0);

    uint8_t addr_val[6] = {0};
    // 复制BLE地址
    rc = ble_hs_id_copy_addr(blehr_addr_type, addr_val, NULL);
    assert(rc == 0);

    // 输出设备地址
    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val); // 这里注释掉的函数用于打印地址，可以取消注释使用
    MODLOG_DFLT(INFO, "\n");

    // 开始广播
    blehr_advertise();
}

// BLE重置事件处理函数，用于在BLE主机重置时执行的操作
static void blehr_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

// BLE主机任务函数，用于运行NimBLE协议栈
void blehr_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    // 运行NimBLE协议栈
    nimble_port_run();
    // 停止NimBLE协议栈
    nimble_port_freertos_deinit();
}

// 应用程序BLE初始化函数
void app_ble_init(void)
{
    get_new_devicename(device_name,length);

    // 初始化NimBLE端口
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        MODLOG_DFLT(ERROR, "Failed to init nimble %d \n", err);
        return;
    }

    // 初始化NimBLE主机配置
    ble_hs_cfg.sync_cb = blehr_on_sync;
    ble_hs_cfg.reset_cb = blehr_on_reset;

    // 初始化GATT服务
    gatt_svr_init();

    // 设置默认设备名称
    ble_svc_gap_device_name_set(device_name);

    // 启动任务
    nimble_port_freertos_init(blehr_host_task);
}
