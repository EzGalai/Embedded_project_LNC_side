#include "config.h"
#include "main.h" /* for xConfigMutexHandle */
#include "cmsis_os.h"
#include <string.h>
#include <stddef.h>

#define CONFIG_FLASH_ADDRESS 0x080FF800u
#define CONFIG_FLASH_BANK    FLASH_BANK_2
#define CONFIG_FLASH_PAGE    255u
#define CONFIG_MAGIC         0x434F4E46u /* "CONF" */

/* Defaults — matching Phase 10's original hardcoded thresholds exactly */
#define DEFAULT_TEMP_NORMAL_LOW      180
#define DEFAULT_TEMP_NORMAL_HIGH     280
#define DEFAULT_TEMP_WARNING_LOW     100
#define DEFAULT_TEMP_WARNING_HIGH    350
#define DEFAULT_HUMIDITY_NORMAL_MIN   30
#define DEFAULT_HUMIDITY_WARNING_MIN  15
#define DEFAULT_LIGHT_NORMAL_MIN     500
#define DEFAULT_LIGHT_WARNING_MIN    200
#define DEFAULT_BATTERY_NORMAL_MIN  2500
#define DEFAULT_BATTERY_WARNING_MIN 2000

static ConfigRecord_t g_current;

/**
 * @brief Software CRC-32 (IEEE 802.3 polynomial), bit-by-bit. Only used to
 * validate the rarely-written Flash config record — not performance-
 * sensitive, so no table-driven speedup is needed.
 */
static uint32_t Crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
        }
    }
    return ~crc;
}

static void SetDefaults(ConfigRecord_t *record)
{
    record->magic              = CONFIG_MAGIC;
    record->tempNormalLow      = DEFAULT_TEMP_NORMAL_LOW;
    record->tempNormalHigh     = DEFAULT_TEMP_NORMAL_HIGH;
    record->tempWarningLow     = DEFAULT_TEMP_WARNING_LOW;
    record->tempWarningHigh    = DEFAULT_TEMP_WARNING_HIGH;
    record->humidityNormalMin  = DEFAULT_HUMIDITY_NORMAL_MIN;
    record->humidityWarningMin = DEFAULT_HUMIDITY_WARNING_MIN;
    record->lightNormalMin     = DEFAULT_LIGHT_NORMAL_MIN;
    record->lightWarningMin    = DEFAULT_LIGHT_WARNING_MIN;
    record->batteryNormalMin   = DEFAULT_BATTERY_NORMAL_MIN;
    record->batteryWarningMin  = DEFAULT_BATTERY_WARNING_MIN;
    record->crc32 = Crc32((const uint8_t *)record, offsetof(ConfigRecord_t, crc32));
}

/**
 * @brief Erases the config page and writes record, padded to a whole number
 * of 64-bit doublewords — STM32L4 Flash can only be programmed a doubleword
 * at a time.
 */
static void WriteRecordToFlash(const ConfigRecord_t *record)
{
    uint8_t buffer[32] = {0}; /* rounded up from sizeof(ConfigRecord_t) to a multiple of 8 */
    memcpy(buffer, record, sizeof(ConfigRecord_t));

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInit;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Banks = CONFIG_FLASH_BANK;
    eraseInit.Page = CONFIG_FLASH_PAGE;
    eraseInit.NbPages = 1;
    uint32_t pageError;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);

    for (uint32_t offset = 0; offset < sizeof(buffer); offset += 8) {
        uint64_t doubleword;
        memcpy(&doubleword, buffer + offset, 8);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CONFIG_FLASH_ADDRESS + offset, doubleword);
    }

    HAL_FLASH_Lock();
}

void Config_LoadFromFlash(void)
{
    const ConfigRecord_t *flashRecord = (const ConfigRecord_t *)CONFIG_FLASH_ADDRESS;
    uint32_t computedCrc = Crc32((const uint8_t *)flashRecord, offsetof(ConfigRecord_t, crc32));

    ConfigRecord_t record;
    if (flashRecord->magic == CONFIG_MAGIC && flashRecord->crc32 == computedCrc) {
        memcpy(&record, flashRecord, sizeof(ConfigRecord_t));
    } else {
        SetDefaults(&record);
        WriteRecordToFlash(&record);
    }

    osMutexAcquire(xConfigMutexHandle, osWaitForever);
    memcpy(&g_current, &record, sizeof(ConfigRecord_t));
    osMutexRelease(xConfigMutexHandle);
}

void Config_GetCurrent(ConfigRecord_t *outConfig)
{
    osMutexAcquire(xConfigMutexHandle, osWaitForever);
    memcpy(outConfig, &g_current, sizeof(ConfigRecord_t));
    osMutexRelease(xConfigMutexHandle);
}

ProtoStatus_t Config_ApplyUpdate(uint8_t tag, const uint8_t *value, uint16_t valueLen)
{
    ConfigRecord_t updated;
    Config_GetCurrent(&updated);

    const uint8_t *field;
    uint16_t fieldLen;

    switch (tag) {
        case PROTO_TAG_SET_TEMP_NORMAL_RANGE:
        case PROTO_TAG_SET_TEMP_WARNING_RANGE: {
            uint16_t rawLow, rawHigh;
            if (Protocol_FindField(value, valueLen, PROTO_FIELD_TEMP_LOW, &field, &fieldLen) != PROTO_OK || fieldLen < 2) {
                return PROTO_STATUS_INTERNAL_ERROR;
            }
            Protocol_GetU16(field, &rawLow);
            if (Protocol_FindField(value, valueLen, PROTO_FIELD_TEMP_HIGH, &field, &fieldLen) != PROTO_OK || fieldLen < 2) {
                return PROTO_STATUS_INTERNAL_ERROR;
            }
            Protocol_GetU16(field, &rawHigh);

            int16_t low = (int16_t)rawLow;
            int16_t high = (int16_t)rawHigh;
            if (low > high) {
                return PROTO_STATUS_INVALID_RANGE;
            }
            if (tag == PROTO_TAG_SET_TEMP_NORMAL_RANGE) {
                updated.tempNormalLow = low;
                updated.tempNormalHigh = high;
            } else {
                updated.tempWarningLow = low;
                updated.tempWarningHigh = high;
            }
            break;
        }

        case PROTO_TAG_SET_HUMIDITY_NORMAL_MIN:
        case PROTO_TAG_SET_HUMIDITY_WARNING_MIN: {
            if (Protocol_FindField(value, valueLen, PROTO_FIELD_HUMIDITY_MIN, &field, &fieldLen) != PROTO_OK || fieldLen < 1) {
                return PROTO_STATUS_INTERNAL_ERROR;
            }
            uint8_t min = field[0];
            if (tag == PROTO_TAG_SET_HUMIDITY_NORMAL_MIN) updated.humidityNormalMin = min;
            else updated.humidityWarningMin = min;
            break;
        }

        case PROTO_TAG_SET_LIGHT_NORMAL_MIN:
        case PROTO_TAG_SET_LIGHT_WARNING_MIN: {
            uint16_t min;
            if (Protocol_FindField(value, valueLen, PROTO_FIELD_LIGHT_MIN, &field, &fieldLen) != PROTO_OK || fieldLen < 2) {
                return PROTO_STATUS_INTERNAL_ERROR;
            }
            Protocol_GetU16(field, &min);
            if (tag == PROTO_TAG_SET_LIGHT_NORMAL_MIN) updated.lightNormalMin = min;
            else updated.lightWarningMin = min;
            break;
        }

        case PROTO_TAG_SET_BATTERY_NORMAL_MIN:
        case PROTO_TAG_SET_BATTERY_WARNING_MIN: {
            uint16_t min;
            if (Protocol_FindField(value, valueLen, PROTO_FIELD_BATTERY_MIN, &field, &fieldLen) != PROTO_OK || fieldLen < 2) {
                return PROTO_STATUS_INTERNAL_ERROR;
            }
            Protocol_GetU16(field, &min);
            if (tag == PROTO_TAG_SET_BATTERY_NORMAL_MIN) updated.batteryNormalMin = min;
            else updated.batteryWarningMin = min;
            break;
        }

        default:
            return PROTO_STATUS_INTERNAL_ERROR;
    }

    updated.magic = CONFIG_MAGIC;
    updated.crc32 = Crc32((const uint8_t *)&updated, offsetof(ConfigRecord_t, crc32));

    WriteRecordToFlash(&updated);

    osMutexAcquire(xConfigMutexHandle, osWaitForever);
    memcpy(&g_current, &updated, sizeof(ConfigRecord_t));
    osMutexRelease(xConfigMutexHandle);

    return PROTO_STATUS_SUCCESS;
}