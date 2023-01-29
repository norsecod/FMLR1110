#include "settings.h"

#include "string.h"

#include "smtc_hal_dbg_trace.h"

settings_t settings;

static uint8_t deveui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t joineui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t nwkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t region = 0x01; // 0x01 = EU868

void settings_init( void ) {
    memcpy(settings.deveui, deveui, sizeof(deveui));
    memcpy(settings.joineui, joineui, sizeof(joineui));
    memcpy(settings.nwkey, nwkey, sizeof(nwkey));
    settings.region = region;
}

void settings_print( void ) {
    SMTC_HAL_TRACE_INFO("Settings:\n"); \
    SMTC_HAL_TRACE_INFO(" - deveui:                   %02X%02X%02X%02X%02X%02X%02X%02X\n", settings.deveui[0], settings.deveui[1], settings.deveui[2], settings.deveui[3], settings.deveui[4], settings.deveui[5], settings.deveui[6], settings.deveui[7]);
    SMTC_HAL_TRACE_INFO(" - joineui:                  %02X%02X%02X%02X%02X%02X%02X%02X\n", settings.joineui[0], settings.joineui[1], settings.joineui[2], settings.joineui[3], settings.joineui[4], settings.joineui[5], settings.joineui[6], settings.joineui[7]);
    SMTC_HAL_TRACE_INFO(" - nwkey:                    %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", settings.nwkey[0], settings.nwkey[1], settings.nwkey[2], settings.nwkey[3], settings.nwkey[4], settings.nwkey[5], settings.nwkey[6], settings.nwkey[7], settings.nwkey[8], settings.nwkey[9], settings.nwkey[10], settings.nwkey[11], settings.nwkey[12], settings.nwkey[13], settings.nwkey[14], settings.nwkey[15]);
    SMTC_HAL_TRACE_INFO(" - region:                   %u\n", settings.region);
}
