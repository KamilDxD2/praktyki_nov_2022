#pragma once

#ifndef _BLUETOOTH__HID_H_INCLUDED
#define _BLUETOOTH__HID_H_INCLUDED

#include "freertos/FreeRTOS.h"
#include "esp_hidd_api.h"

enum bluetooth_device_state {
    BT_DEVICE_CONNECTED,
    BT_DEVICE_DISCONNECTED
};

typedef void (*bluetooth_device_state_cb_t)(enum bluetooth_device_state state, esp_bd_addr_t bt_addr);

void bluetooth_set_device_state_cb(bluetooth_device_state_cb_t cb);
BaseType_t bluetooth_start_autoreconnect_task(void);
void bluetooth_hid_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#endif // _BLUETOOTH__HID_H_INCLUDED