
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_system.h"



void ble_function_select(uint16_t status){
    switch (status)
    {
    case 1:
      esp_restart();
      /* code */
      break;
    case 2:
      
    break;
    
    default:
      break;
    };
}