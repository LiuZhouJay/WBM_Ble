#ifndef H_APP_LOCALDATA_
#define H_APP_LOCALDATA_

#ifdef __cplusplus
extern "C" {
#endif

void get_new_macaddr(uint32_t last_macaddr);
void get_new_devicename(char* last_devicename,size_t length);
void modification_macaddr(uint32_t value);
void modification_name(const char* value);


#ifdef __cplusplus
}
#endif

#endif /* H_APP_TURMASS_ */