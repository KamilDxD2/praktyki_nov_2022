idf_component_register(SRCS
    src/display/DEV_Config.cpp
    src/display/EPD_2in13_V3.cpp
    src/display/font8.cpp
    src/display/font16.cpp
    src/display/GUI_Paint.cpp
    src/display/display_main.cpp
    src/config/persistconfig.cpp
    src/bluetooth/main.c
    src/bluetooth/hid_dev.c
    src/bluetooth/esp_hidd_prf_api.c
    src/bluetooth/hid_device_le_prf.c
    src/arduino_init.c
    INCLUDE_DIRS ".")
