#ifndef H_APP_OTA_
#define H_APP_OTA_


#ifdef __cplusplus
extern "C" {
#endif

void ota_init(void);
void ota_upgrade(const void *data, size_t size);



#ifdef __cplusplus
}
#endif

#endif