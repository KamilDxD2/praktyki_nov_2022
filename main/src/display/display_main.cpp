#include "display_main.h"

uint8_t wifiIcon[] = {
    0xff,   0xff,   0xff,   0x9f,   0xfc,   0xef,   0xff,   
    0x77,   0xf3,   0xb7,   0xfd,   0xbb,   0x9e,   0xdb,   
    0x0e,   0xdb,   0x0e,   0xdb,   0x9e,   0xdb,   0xfd,   
    0xbb,   0xf3,   0xb7,   0xff,   0x77,   0xfc,   0xef,   
    0xff,   0x9f,   0xff,   0xff,   0xff,   0xff,   0xff,
    0x01, 	0x1e, 	0x1f, 	0x1c, 	0x1e, 	0x54, 	0x40,
	0x5f, 	0x10, 	0x1e, 	0x18, 	0x15, 	0x05, 	0x1f, 
	0x01, 	0x17, 	0x57, 	0x1d, 	0x12, 	0x08, 	0x15, 
	0x0b, 	0x1d, 	0xae, 	0xe8, 	0xed, 	0xac
};

QueueHandle_t displayUpdateQueue = NULL;

uint8_t image[128 * 256 / 8];

char Display::currentTop[256];
char Display::currentMain[256];
char Display::currentBottom[256];
char wifiIconVisible;
Display::Position centerText(int textLength, int offsetY, int charWidth, int charHeight){
    Display::Position pos;
    int textWidth = (charWidth+1+4) * textLength - textLength;
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
                strcpy(Display::currentBottom, incoming.bottom);
                free(incoming.bottom);
            }
            if(incoming.main != NULL){
                strcpy(Display::currentMain, incoming.main);
                free(incoming.main);
            }
            if(incoming.top != NULL){
                strcpy(Display::currentTop, incoming.top);
                free(incoming.top);
            }
            centered = centerText(strlen(Display::currentBottom), 45, 5*4, 8*4);
            Paint_DrawString_EN(centered.x, centered.y, Display::currentBottom, &Font8, 1, 0, 4);
            centered = centerText(strlen(Display::currentMain), 0, 5*4, 8*4);
            Paint_DrawString_EN(centered.x, centered.y, Display::currentMain, &Font8, 1, 0, 4);
            centered = centerText(strlen(Display::currentTop), -45, 5*4, 8*4);
            Paint_DrawString_EN(centered.x, centered.y, Display::currentTop, &Font8, 1, 0, 4);
            if(incoming.wifiIconVisible != 0xFF){
                wifiIconVisible = incoming.wifiIconVisible;
            }
            if(wifiIconVisible){
                Paint_DrawImage(wifiIcon, 0, 0, 16, 16);
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
    allocated.wifiIconVisible = reference.wifiIconVisible;
    xQueueSend(displayUpdateQueue, &allocated, 0);
}
