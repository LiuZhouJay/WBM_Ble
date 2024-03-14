#include "driver/uart.h"
#include "app_rs485.h"
#include "app_turmass.h"
#include <string.h>

#if 0
#include <stdio.h>
#define BALANCE_LOG(...) printf(__VA_ARGS__)
#else
#define BALANCE_LOG(...)
#endif


static QueueHandle_t uart0_queue;

static uint8_t data[512];
static void rs485_task(void *arg)
{
    uart_event_t event;
    int len;

    while (1)
    {
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                if(event.size<=sizeof(data))
                {
                    len = uart_read_bytes(UART_NUM_0, data, event.size, 0);
                    BALANCE_LOG("len:%d\r\n",len);
                    if(len>=120)
                    {
                        tkm_101_write_queue((const char*) data, len);
                        // vTaskDelay(pdMS_TO_TICKS(1200));
                    }
                    else
                    {
                        tkm_101_write_queue((const char*) data, len);
                    }
                }
                break;
            case UART_FIFO_OVF:
                uart_flush_input(UART_NUM_0);
                xQueueReset(uart0_queue);
                break;
            case UART_BUFFER_FULL:
                uart_flush_input(UART_NUM_0);
                xQueueReset(uart0_queue);
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
                break;
            }
        }
    }
}

int app_rs485_init(void)
{
    uart_config_t uart_config;

    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = SOC_MOD_CLK_APB;

    uart_driver_install(UART_NUM_0, 1024, 1024*3, 128, &uart0_queue, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 21, 20, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(rs485_task, "rs485_task", 2048, NULL, 10, NULL);
    return 0;
}
