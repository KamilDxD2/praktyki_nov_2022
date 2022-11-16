#include "display_main.h"

QueueHandle_t displayUpdateQueue = NULL;

uint8_t image[128 * 256 / 8];

Display::Position centerText(int textLength, int offsetY, int charWidth, int charHeight){
    Display::Position pos;
    int textWidth = (charWidth+1) * textLength;
    pos.x = 0;
    pos.x += (EPD_2in13_V3_HEIGHT - textWidth) / 2;
    pos.y = 0;
    pos.y += (EPD_2in13_V3_WIDTH - charHeight) / 2;
    pos.y += offsetY;
    return pos;
}

void mainDisplayTask(void* pvParameters){
    Display::DisplayUpdate incoming;

    for(;;){
        if(xQueueReceive(displayUpdateQueue, &incoming, portMAX_DELAY) == pdTRUE){
            // 'incoming' now has the display update.
            Paint_Clear(0xFF);
            Display::Position centered;
            if(incoming.bottom != NULL){
                centered = centerText(strlen(incoming.bottom), 45, 5*4, 8*4);
                Paint_DrawString_EN(centered.x, centered.y, incoming.bottom, &Font8, 1, 0, 4);
                free(incoming.bottom);
            }
            if(incoming.main != NULL){
                centered = centerText(strlen(incoming.main), 0, 5*8, 8*8);
                Paint_DrawString_EN(centered.x, centered.y, incoming.main, &Font8, 1, 0, 8);
                free(incoming.main);
            }
            if(incoming.top != NULL){
                centered = centerText(strlen(incoming.top), -45, 5*4, 8*4);
                Paint_DrawString_EN(centered.x, centered.y, incoming.top, &Font8, 1, 0, 4);
                free(incoming.top);
            }
            EPD_2in13_V3_Display(image);
        }
    }
}

void Display::init_display(){
    DEV_Module_Init();
    EPD_2in13_V3_Init();
    EPD_2in13_V3_Clear();
    Paint_NewImage(image, EPD_2in13_V3_WIDTH, EPD_2in13_V3_HEIGHT, 90, 0xFF);

    displayUpdateQueue = xQueueCreate(10, sizeof(Display::DisplayUpdate));
    xTaskHandle handle;
    xTaskCreate(&mainDisplayTask, "display", 2*1024, NULL, configMAX_PRIORITIES - 3, &handle);
}

void Display::queue_display_update(const Display::DisplayUpdate &reference){
    Display::DisplayUpdate allocated;
    int length;

    if(reference.bottom){
        length = strlen(reference.bottom);
        allocated.bottom = (char*) malloc(length + 1);
        strcpy(allocated.bottom, reference.bottom);
    }
    if(reference.top){
        length = strlen(reference.top);
        allocated.top = (char*) malloc(length + 1);
        strcpy(allocated.top, reference.top);
    }
    if(reference.main){
        length = strlen(reference.main);
        allocated.main = (char*) malloc(length + 1);
        strcpy(allocated.main, reference.main);
    }
    xQueueSend(displayUpdateQueue, &allocated, 0);
}
