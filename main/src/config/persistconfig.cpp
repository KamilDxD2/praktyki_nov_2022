#include "persistconfig.h"

nvs_handle_t storageHandle;

void Config::init(){
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &storageHandle));
}

void Config::format(void){
    nvs_erase_all(storageHandle);
}

uint32_t Config::getUint32(const char *name){
    uint32_t e;
    ESP_ERROR_CHECK(nvs_get_u32(storageHandle, name, &e));
    return e;
}
uint16_t Config::getUint16(const char *name){
    uint16_t e;
    ESP_ERROR_CHECK(nvs_get_u16(storageHandle, name, &e));
    return e;
}
uint8_t Config::getUint8(const char *name){
    uint8_t e;
    ESP_ERROR_CHECK(nvs_get_u8(storageHandle, name, &e));
    return e;
}
int32_t Config::getInt32(const char *name){
    int32_t e;
    ESP_ERROR_CHECK(nvs_get_i32(storageHandle, name, &e));
    return e;
}
int16_t Config::getInt16(const char *name){
    int16_t e;
    ESP_ERROR_CHECK(nvs_get_i16(storageHandle, name, &e));
    return e;
}
int8_t Config::getInt8(const char *name){
    int8_t e;
    ESP_ERROR_CHECK(nvs_get_i8(storageHandle, name, &e));
    return e;
}
char* Config::getString(const char *name){
    size_t length;
    ESP_ERROR_CHECK(nvs_get_str(storageHandle, name, NULL, &length));
    char* data = (char*) malloc(length);
    ESP_ERROR_CHECK(nvs_get_str(storageHandle, name, data, &length));
    return data;
}
uint8_t* Config::getBlob(const char *name){
    size_t length;
    ESP_ERROR_CHECK(nvs_get_blob(storageHandle, name, NULL, &length));
    uint8_t* data = (uint8_t*) malloc(length);
    ESP_ERROR_CHECK(nvs_get_blob(storageHandle, name, data, &length));
    return data;
}


bool Config::hasUint32(const char *name){
    uint32_t e;
    return nvs_get_u32(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasUint16(const char *name){
    uint16_t e;
    return nvs_get_u16(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasUint8(const char *name){
    uint8_t e;
    return nvs_get_u8(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasInt32(const char *name){
    int32_t e;
    return nvs_get_i32(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasInt16(const char *name){
    int16_t e;
    return nvs_get_i16(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasInt8(const char *name){
    int8_t e;
    return nvs_get_i8(storageHandle, name, &e) == ESP_OK;
}
bool Config::hasString(const char *name){
    size_t length;
    return nvs_get_str(storageHandle, name, NULL, &length) == ESP_OK;
}
bool Config::hasBlob(const char *name){
    size_t length;
    return nvs_get_blob(storageHandle, name, NULL, &length) == ESP_OK;
}


void Config::setUint32(const char* name, uint32_t value){
    ESP_ERROR_CHECK(nvs_set_u32(storageHandle, name, value));
}
void Config::setUint16(const char* name, uint16_t value){
    ESP_ERROR_CHECK(nvs_set_u16(storageHandle, name, value));
}
void Config::setUint8(const char* name, uint8_t value){
    ESP_ERROR_CHECK(nvs_set_u8(storageHandle, name, value));
}
void Config::setInt32(const char* name, int32_t value){
    ESP_ERROR_CHECK(nvs_set_i32(storageHandle, name, value));
}
void Config::setInt16(const char* name, int16_t value){
    ESP_ERROR_CHECK(nvs_set_i16(storageHandle, name, value));
}
void Config::setInt8(const char* name, int8_t value){
    ESP_ERROR_CHECK(nvs_set_i8(storageHandle, name, value));
}
void Config::setString(const char* name, char* content){
    ESP_ERROR_CHECK(nvs_set_str(storageHandle, name, content));
}
void Config::setBlob(const char* name, uint8_t *buffer, int length){
    ESP_ERROR_CHECK(nvs_set_blob(storageHandle, name, buffer, length));
}