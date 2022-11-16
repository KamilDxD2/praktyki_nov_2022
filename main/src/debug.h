#pragma once

#ifndef _DEBUG_H_INCLUDED
#define _DEBUG_H_INCLUDED

#include "esp_log.h"
#include "esp_err.h"

#define DT_TAG "DeskThing"

#define DT_LOG(logfunc, format, ...) logfunc(DT_TAG, "[%s:%d] %s(): " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
// uncomment to disable debugs
// #define DT_LOG(...)

#define DT_DEBUG(format, ...)   DT_LOG(ESP_LOGD, format, ##__VA_ARGS__)
#define DT_INFO(format, ...)    DT_LOG(ESP_LOGI, format, ##__VA_ARGS__)
#define DT_WARNING(format, ...) DT_LOG(ESP_LOGW, format, ##__VA_ARGS__)
#define DT_ERROR(format, ...)   DT_LOG(ESP_LOGE, format, ##__VA_ARGS__)

#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
#define DT_ESP_ERR_FORMAT "%s"
#define DT_ESP_ERR(code) esp_err_to_name(code)
#else
#define DT_ESP_ERR_FORMAT "%d"
#define DT_ESP_ERR(code) (code)
#endif // CONFIG_ESP_ERR_TO_NAME_LOOKUP

// Bluetooth address formatting
#define DT_BT_ADDR_FORMAT "%02x:%02x:%02x:%02x:%02x:%02x"
#define DT_BT_ADDR(array) array[0], array[1], array[2], array[3], array[4], array[5]

#endif