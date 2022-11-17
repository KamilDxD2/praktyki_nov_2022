#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "nvs.h"

namespace Config{
    void init(void);
    void format(void);

    bool hasUint32(const char* name);
    bool hasUint16(const char* name);
    bool hasUint8(const char* name);
    bool hasInt32(const char* name);
    bool hasInt16(const char* name);
    bool hasInt8(const char* name);
    bool hasString(const char* name);
    bool hasBlob(const char* name);

    uint32_t getUint32(const char* name);
    uint16_t getUint16(const char* name);
    uint8_t getUint8(const char* name);
    int32_t getInt32(const char* name);
    int16_t getInt16(const char* name);
    int8_t  getInt8(const char* name);
    char* getString(const char* name);
    uint8_t* getBlob(const char* name);

    void setUint32(const char* name, uint32_t v);
    void setUint16(const char* name, uint16_t v);
    void setUint8(const char* name, uint8_t v);
    void setInt32(const char* name, int32_t v);
    void setInt16(const char* name, int16_t v);
    void setInt8(const char* name, int8_t v);
    void setString(const char* name, char* string);
    void setBlob(const char* name, uint8_t *buffer, int length);
}
