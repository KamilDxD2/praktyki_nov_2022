#pragma once

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../debug.h"
#include <map>
#include <vector>
#include "time.h"
#include "ArduinoJson.h"

struct WSConnection{
    int fd;
    httpd_handle_t handle;
    int lastPingTime;
};

typedef void(*ws_state_update_cb_t)(uint8_t* incoming, int length);
typedef void(*ws_new_connection_cb_t)();
esp_err_t startHttpServer();
void broadcastStringToWS(String &data);
void setStateUpdateCallback(ws_state_update_cb_t cb);
void setNewConnectionCallback(ws_new_connection_cb_t cb);
