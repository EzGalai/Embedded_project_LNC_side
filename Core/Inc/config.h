#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_
#include <stdint.h>
#include "protocol.h"

typedef struct {
    uint32_t magic;
    int16_t  tempNormalLow,  tempNormalHigh;
    int16_t  tempWarningLow, tempWarningHigh;
    uint8_t  humidityNormalMin,  humidityWarningMin;
    uint16_t lightNormalMin,     lightWarningMin;
    uint16_t batteryNormalMin,   batteryWarningMin;
    uint32_t crc32;
} ConfigRecord_t;

/**
 * @brief Loads the config record from Flash into the in-RAM cache. Called
 * once by vInitTask at boot. If Flash is invalid (erased, or a partial write
 * left by a power loss mid-save), writes and uses defaults instead.
 */
void Config_LoadFromFlash(void);

/**
 * @brief Thread-safe read of the current in-RAM config cache.
 * @param outConfig Set to a copy of the current record.
 */
void Config_GetCurrent(ConfigRecord_t *outConfig);

/**
 * @brief Validates a SET_* command's new value, erases+rewrites the Flash
 * page, and updates the in-RAM cache.
 * @param tag Which SET_* tag arrived (selects which field(s) to update).
 * @param value The command's Value bytes.
 * @param valueLen Length of value.
 * @return The STATUS to reply with via CONFIG_ACK.
 */
ProtoStatus_t Config_ApplyUpdate(uint8_t tag, const uint8_t *value, uint16_t valueLen);

#endif

