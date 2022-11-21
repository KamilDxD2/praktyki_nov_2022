#pragma once

#ifndef _BLUETOOTH__MAIN_H_INCLUDED
#define _BLUETOOTH__MAIN_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

enum bluetooth_device_state {
    BT_DEVICE_CONNECTED,
    BT_DEVICE_DISCONNECTED
};

typedef void (*bluetooth_device_state_cb_t)(enum bluetooth_device_state state);

void bluetooth_set_device_state_cb(bluetooth_device_state_cb_t cb);
int bluetooth_init(void);
void bluetooth_set_device_name(const char *name);
int bluetooth_start(void);

#ifdef __cplusplus
}
#endif

#endif // _BLUETOOTH__MAIN_H_INCLUDED
