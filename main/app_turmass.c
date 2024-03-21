#include <string.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "app_turmass.h"
#include "freertos/queue.h"
#include "app_gatt_svr.c"

#include "app_localdata.h"

#include <ctype.h>
#define DEBUG_FLAG 1

#define USE_DIRECT_TRANSFER 0

#if 1
#include <stdio.h>
#define BALANCE_LOG(...) printf(__VA_ARGS__)
#else
#define BALANCE_LOG(...)
#endif

static QueueHandle_t uart1_receive_queue;
QueueHandle_t turmass_send_cache_queue = NULL;
static SemaphoreHandle_t turmass_return_ok_semaphore = NULL;
static SemaphoreHandle_t turmass_return_sendfinish_semaphore = NULL;

uint32_t Terminal_mac = 0x11111111;

// data.c
void Turmass_send_data_ble(const char *data) {
    ble_receive_data(data);
}


void tkm_101_write_bytes(const char *data, size_t len)
{
    uart_write_bytes(UART_NUM_1, (const char *)data, len);
}

void tkm_101_write_str(const char *data)
{
    uart_write_bytes(UART_NUM_1, (const char *)data, strlen(data));
}

volatile char response_str[64] = {"AT_OK"};

void tkm_101_write_at_send(const char *data, size_t len)
{
    static char buf[1024];
    static char send_buf[1024 + 128];
    static char temp[8];
    memset(buf, 0, sizeof(buf));
    memset(temp, 0, sizeof(temp));
    memset(send_buf, 0, sizeof(send_buf));
    for (int i = 0; i < len; i++)
    {
        sprintf(temp, "%02X", data[i]);
        strcat(buf, temp);
    }
    sprintf(send_buf, "AT+SENDB=%s\r\n", buf);
    uart_write_bytes(UART_NUM_1, (const char *)send_buf, strlen(send_buf));
}

void tkm_101_write_at(const char *data, const char *desired_response)
{
    if (desired_response)
    {
        strncpy((char *)&response_str[0], desired_response, sizeof(response_str));
        xSemaphoreTake(turmass_return_ok_semaphore, 0);
        uart_write_bytes(UART_NUM_1, data, strlen(data));
        xSemaphoreTake(turmass_return_ok_semaphore, pdMS_TO_TICKS(15000));
        memset((void *)response_str, 0, sizeof(response_str));
    }
    else
    {
        memset((void *)response_str, 0, sizeof(response_str));
        uart_write_bytes(UART_NUM_1, data, strlen(data));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void tkm_101_wait(const char *desired_response)
{
    strncpy((char *)&response_str[0], desired_response, sizeof(response_str));
    xSemaphoreTake(turmass_return_ok_semaphore, pdMS_TO_TICKS(15000));
    memset((void *)response_str, 0, sizeof(response_str));
}

void tkm_101_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 82 ---关闭透传模式，即 AT 命令模式
    tkm_101_write_at("AT+WORKMODE=82\r\n", "AT_PARAM_ERROR");
    // 恢复默认参数
    tkm_101_write_at("AT+RSTPARA\r\n", "AT_OK");
    // 复位
    tkm_101_write_at("AT+RST\r\n", "TurMass");
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 同时设置数据发送频率、数据接收频率、BCN 发送频率及 BCN 接收频率，单位为 Hz
    tkm_101_write_at("AT+FREQ=451125000\r\n", "AT_OK");
    // ????????????32?
    tkm_101_write_at("AT+WORKMODE=32\r\n", "AT_OK");
    // 信道类型为网关、网关信道个数1个、信道列表451.125MHZ
    tkm_101_write_at("AT+CH=0,1,451125000\r\n", "AT_OK");
    // 设置发送和接收无线传输速率模式均为 18，82500bps
    tkm_101_write_at("AT+RATE=18\r\n", "AT_OK");
    /* 最多支持 59 个时隙 */

    tkm_101_write_at("AT+FRAMECFG=8,600,8,600,7,600,8,600,8,600,8,600,8,600,8,600,8,600\r\n", "AT_OK");
    
    tkm_101_write_at("AT+NETSCAN=1\r\n", "PRESEND_READY");
    // tkm_101_write_at("AT+WORKMODE=81\r\n", "AT_OK");
}

struct app_turmass_data_t
{
    uint32_t head;
    uint32_t mac;
    uint32_t sequence;
    uint32_t crc;
    uint32_t len;
    uint8_t buf[580 + 1]; /* max:580+1 ; min:128+1*/
};

void tkm_101_write_queue(const char *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (pdPASS != xQueueSend(turmass_send_cache_queue, &data[i], portMAX_DELAY))
        {
            break;
        }
    }
}

static void turmass_send_task(void *arg)
{
    // static uint8_t buf[600];
    static struct app_turmass_data_t data;

    data.head = 0xA5A5A5A5;
    data.mac = Terminal_mac;
    data.sequence = 0;
    tkm_101_init();
    while (1)
    {
        while (1)
        {
            if (pdTRUE == xQueueReceive(turmass_send_cache_queue, &(data.buf[data.len]), pdMS_TO_TICKS(500)))
            {
                data.len++;
                if (data.len >= (sizeof(data.buf) - 1))
                {
                    break;
                }
            }
            //判断在这之前是否已经接收过值
            else if (data.len)
            {
                break;
            }
        }
#if USE_DIRECT_TRANSFER
        tkm_101_write_bytes((const char *)data.buf, data.len);
#else
        tkm_101_wait("PRESEND_READY");
        xSemaphoreTake(turmass_return_sendfinish_semaphore, 0);
        tkm_101_write_at_send((const char *)&data, data.len + 20);
        memset(data.buf, 0, sizeof(data.buf));
        xSemaphoreTake(turmass_return_sendfinish_semaphore, pdMS_TO_TICKS(10000));

#endif
        data.sequence++;
        data.len = 0;
    }
}
int j;
static void turmass_receive_task(void *arg)
{
    uart_event_t event;
    static uint8_t data[600];
    int len;
    char *Data_head = NULL;
    int Data_start = 0;
    char *Data_end = NULL;
    while (1)
    {
        if (xQueueReceive(uart1_receive_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
#if USE_DIRECT_TRANSFER
                len = uart_read_bytes(UART_NUM_1, data, event.size, 0);
                uart_write_bytes(UART_NUM_0, (const char *)data, len);
#else
                if (event.size <= sizeof(data))
                {
                    len = uart_read_bytes(UART_NUM_1, data, event.size, 0);
                    data[len] = 0;
                    if (response_str[0] != 0)
                    {
                        if (strstr((char *)(data), (char *)response_str) != NULL)
                        {
                            // BALANCE_LOG("xSemaphoreGive( turmass_return_ok_semaphore );\r\n");
                            BALANCE_LOG("%s",data);
                            Turmass_send_data_ble((char *)data);
                            xSemaphoreGive(turmass_return_ok_semaphore);
                        }
                    }
                    if (strstr((char *)(data), (char *)"+SEND_FINISH!") != NULL)
                    {
                        BALANCE_LOG("%s",data);
                        Turmass_send_data_ble((char *)data);
                        xSemaphoreGive(turmass_return_sendfinish_semaphore);
                    }
                    if (Data_start == 0)
                    {
                        Data_head = strstr((char *)(data), ", Data ");
                    }
                    else
                    {
                        Data_end = strstr((char *)(data), "\r\n");
                    }

                    if (Data_head != NULL)
                    {
                        // BALANCE_LOG("have Data(TO DO)pos:%s\r\n",&pos[7]);
                        uart_write_bytes(UART_NUM_0, (const char *)&Data_head[0], len - 2 - (int32_t)((uint32_t)Data_head - (uint32_t)data));
                        Turmass_send_data_ble((const char*) &Data_head[7]);
                        Data_start = 1;
                        Data_head = NULL;
                    }
                    else if (Data_start)
                    {         
                        uart_write_bytes(UART_NUM_0, (const char*) data, len);
                        if (Data_end)
                        {
                            Data_end = NULL;
                            Data_start = 0;
                        }
                    }
                    else
                    {
                        
                        // BALANCE_LOG("Else:%s",data);
                        
                    }
                }
#endif
                break;
            case UART_FIFO_OVF:
                uart_flush_input(UART_NUM_1);
                xQueueReset(uart1_receive_queue);
                break;
            case UART_BUFFER_FULL:
                uart_flush_input(UART_NUM_1);
                xQueueReset(uart1_receive_queue);
                break;
            case UART_BREAK:
                break;
            case UART_PARITY_ERR:
                break;
            case UART_FRAME_ERR:
                break;
            case UART_PATTERN_DET:
                break;
            default:
                BALANCE_LOG("uart event type: %d", event.type);
                break;
            }
        }
    }
}

int app_turmass_init(void)
{
    get_new_macaddr(Terminal_mac);

    uart_config_t uart_config;

    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = SOC_MOD_CLK_APB;

    turmass_send_cache_queue = xQueueCreate(1024 * 3, 1);
    turmass_return_ok_semaphore = xSemaphoreCreateBinary();
    turmass_return_sendfinish_semaphore = xSemaphoreCreateBinary();

    uart_driver_install(UART_NUM_1, 1024 * 3, 1024, 15, &uart1_receive_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 6, 7, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(turmass_receive_task, "turmass_receive_task", 2048, NULL, 10, NULL);
    xTaskCreate(turmass_send_task, "turmass_send_task", 2048, NULL, 10, NULL);
    return 0;
}
