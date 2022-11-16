#pragma once

#ifndef _BLUETOOTH__MAIN_H_INCLUDED
#define _BLUETOOTH__MAIN_H_INCLUDED

int bluetooth_init(void);
void bluetooth_set_device_name(const char *name);
int bluetooth_start(void);

#endif // _BLUETOOTH__MAIN_H_INCLUDED
