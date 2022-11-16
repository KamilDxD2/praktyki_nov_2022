#include "debug.h"
#include "bluetooth/main.h"

void app_main(void) {
    int err = 0;

    DT_INFO("Initializing bluetooth...");
    if ((err = bluetooth_init())) {
        DT_ERROR("Failed to initialize bluetooth, error code: %d", err);
        return;
    }
    DT_INFO("Bluetooth initialized");

    bluetooth_start();
}