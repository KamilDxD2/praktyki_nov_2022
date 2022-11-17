#pragma once

#ifndef _BLUETOOTH__MAIN_H_INCLUDED
#define _BLUETOOTH__MAIN_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int bluetooth_init(void);
void bluetooth_set_device_name(const char *name);
int bluetooth_start(void);

#ifdef __cplusplus
}
#endif

#endif // _BLUETOOTH__MAIN_H_INCLUDED
