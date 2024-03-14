/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "led_strip.h"

#include "app_rs485.h"
#include "app_ble.h"
#include "app_turmass.h"

void app_main(void)
{
    app_rs485_init();
    app_turmass_init();
    app_ble_init();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
// 2B 52 53 54 3A 20 6F 6B 0D 0A 41 54 5F 4F 4B FF 54 75 72 4D 61 73 73 2E 54 75 72 4D 61 73 73 2E 54 75 72 4D 61 73 73 2E 50 32 50 20 41 54 20 43 4D 44 21 0D 0A 