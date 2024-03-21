#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h" 
#include <ctype.h>
#include "esp_system.h"
#include "app_localdata.h"


void get_new_macaddr(uint32_t last_macaddr){
    nvs_handle_t macaddr_handle; 
    esp_err_t err = nvs_open("mac_space", NVS_READWRITE, &macaddr_handle); 
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handlemac!\n", esp_err_to_name(err));
    } else {
        err =  nvs_get_u32 (macaddr_handle, "Terminalmac_key", &last_macaddr);
        if(err != ESP_OK){
            printf(" \n");
            printf("Reading faild mac ... \n");
        }else{
            printf("Terminal_mac = %lu\n", last_macaddr);
        }
        nvs_close(macaddr_handle);
    }
}

void get_new_devicename(char* last_devicename,size_t length){
    /*********打开nvs**********/
    // 打开NVS存储器
    nvs_handle_t my_handle; // 定义一个NVS句柄变量
    esp_err_t err = nvs_open("name_space", NVS_READWRITE, &my_handle); // 以读写模式打开NVS命名空间
    if (err != ESP_OK) {
        // 如果打开失败，打印错误信息
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // modification_name();

        err =  nvs_get_str(my_handle, "device_name_key", last_devicename, &length);
        if(err != ESP_OK){
            printf("Reading faild name ...\n ");
        }else{
            last_devicename[length] = '\0';
            printf("Device name = %s\n", last_devicename);
        }

        // 关闭NVS句柄
        nvs_close(my_handle); // 关闭NVS命名空间
    }
}

void modification_macaddr(uint32_t value){
    nvs_handle_t handle; 
    esp_err_t ret = nvs_open("mac_space", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        printf("Done\n");
    }
    ret = nvs_set_u32(handle, "Terminalmac_key", value); 

    printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");

    printf("Committing updates in NVS ... ");
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        printf("Error (%s) Committing updates in NVS\n", esp_err_to_name(ret));
    } else {
        printf("Done\n");  
    }
    nvs_close(handle);
}


void modification_name(const char* value){
    nvs_handle_t name_handle2;
    esp_err_t ret = nvs_open("name_space", NVS_READWRITE, &name_handle2);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handlename!\n", esp_err_to_name(ret));
    } else {
        printf("Done\n");
    }
    ret = nvs_set_str(name_handle2, "device_name_key", value);
    printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");
    printf("Committing updates in NVS ... ");
    ret = nvs_commit(name_handle2);
    if (ret != ESP_OK) {
        nvs_close(name_handle2);
        printf("Error (%s) Committing updates in NVS\n", esp_err_to_name(ret));
    } else {
        nvs_close(name_handle2);
        printf("Done\n");
        esp_restart();        
    }
}