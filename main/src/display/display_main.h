#pragma once

#include "EPD.h"
#include "DEV_Config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include "GUI_Paint.h"
#include "fonts.h"

namespace Display{
    extern char currentTop[256];
    extern char currentMain[256];
    extern char currentBottom[256];

    struct DisplayUpdate{
        char *top = NULL;
        char *main = NULL;
        char *bottom = NULL;
        char wifiIconVisible = 0xFF;
    };

    struct Position{
        int x;
        int y;
    };

    void init_display(void);
    void queue_display_update(const Display::DisplayUpdate &update);
}
