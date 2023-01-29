#ifndef __EXTENSIONS_DRIVERS_SETTINGS_SETTINGS_H__
#define __EXTENSIONS_DRIVERS_SETTINGS_SETTINGS_H__

#include "stdint.h"

typedef struct {
    uint8_t     deveui[8];
    uint8_t     joineui[8];
    uint8_t     nwkey[16];
    uint8_t     region;
    uint8_t     hash[32];
} settings_t;

void settings_init( void );

void settings_print( void );

#endif // __EXTENSIONS_DRIVERS_SETTINGS_SETTINGS_H__
