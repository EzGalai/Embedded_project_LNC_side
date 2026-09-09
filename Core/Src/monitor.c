/*
 * monitor.c — see monitor.h.
 */

#include "monitor.h"
#include "main.h"
#include "cmsis_os.h"
#include "dht11.h"
#include "adc_sensors.h"
#include <string.h>
#include<stdio.h>

/* Threshold constants — hardcoded until Phase 12's Configuration/Flash
   module exists; these mirror what Config_LoadFromFlash's defaults will
   eventually be. */
#define TEMP_NORMAL_LOW      180  /* 18.0 C, tenths of a degree */
#define TEMP_NORMAL_HIGH     280  /* 28.0 C */
#define TEMP_WARNING_LOW     100  /* 10.0 C */
#define TEMP_WARNING_HIGH    350  /* 35.0 C */
#define HUMIDITY_NORMAL_MIN   30  /* % */
#define HUMIDITY_WARNING_MIN  15  /* % */
#define LIGHT_NORMAL_MIN     500  /* raw ADC */
#define LIGHT_WARNING_MIN    200  /* raw ADC */
#define BATTERY_NORMAL_MIN  2500  /* mV */
#define BATTERY_WARNING_MIN 2000  /* mV */

static MonitorData_t g_latest = {0};

void Monitor_GetLatest(MonitorData_t *outData)
{
    osMutexAcquire(xMonitorCacheMutexHandle, osWaitForever);
    memcpy(outData, &g_latest, sizeof(MonitorData_t));
    osMutexRelease(xMonitorCacheMutexHandle);
}

void Monitor_SetLatest(const MonitorData_t *data)
{
    osMutexAcquire(xMonitorCacheMutexHandle, osWaitForever);
    memcpy(&g_latest, data, sizeof(MonitorData_t));
    osMutexRelease(xMonitorCacheMutexHandle);
}


static ProtoMode_t ClassifyMin(uint16_t value, uint16_t normalMin, uint16_t warningMin)
{
    if (value >= normalMin) return PROTO_MODE_NORMAL;
    if (value >= warningMin) return PROTO_MODE_WARNING;
    return PROTO_MODE_ERROR;
}

static ProtoMode_t ClassifyRange(int16_t value, int16_t normalLow, int16_t normalHigh,
                                  int16_t warningLow, int16_t warningHigh)
{
    if (value >= normalLow && value <= normalHigh) return PROTO_MODE_NORMAL;
    if (value >= warningLow && value <= warningHigh) return PROTO_MODE_WARNING;
    return PROTO_MODE_ERROR;
}

static ProtoMode_t WorstMode(ProtoMode_t a, ProtoMode_t b)
{
    return (b > a) ? b : a; /* relies on NORMAL=0 < WARNING=1 < ERROR=2 */
}

bool Monitor_Sample(void)
{
    MonitorData_t previous;
    Monitor_GetLatest(&previous);

    DHT_Data dhtResult;
    DHT_Status dhtStatus = DHT_ReadData(DHT11_Pin, DHT11_GPIO_Port, &htim6, &dhtResult);


    MonitorData_t data = {0};
    data.timestamp = g_lncClock;

    if (dhtStatus == DHT_OK) {
        data.temperature = (int16_t)(dhtResult.temperature * 10);
        data.humidity = dhtResult.humidity;
    } else {
        /* Read failed this cycle — keep the previous reading rather than a
           false zero, so one bad read doesn't spuriously trip a threshold. */
        data.temperature = previous.temperature;
        data.humidity = previous.humidity;
    }

    data.batteryVoltage = ADC_ReadBatteryVoltage();
    data.light = ADC_ReadLight();

    ProtoMode_t overall = PROTO_MODE_NORMAL;
    overall = WorstMode(overall, ClassifyRange(data.temperature, TEMP_NORMAL_LOW, TEMP_NORMAL_HIGH,
                                                TEMP_WARNING_LOW, TEMP_WARNING_HIGH));
    overall = WorstMode(overall, ClassifyMin(data.humidity, HUMIDITY_NORMAL_MIN, HUMIDITY_WARNING_MIN));
    overall = WorstMode(overall, ClassifyMin(data.light, LIGHT_NORMAL_MIN, LIGHT_WARNING_MIN));
    overall = WorstMode(overall, ClassifyMin(data.batteryVoltage, BATTERY_NORMAL_MIN, BATTERY_WARNING_MIN));
    data.mode = overall;

    bool modeChanged = (previous.mode != data.mode);

    osMutexAcquire(xMonitorCacheMutexHandle, osWaitForever);
    memcpy(&g_latest, &data, sizeof(MonitorData_t));
    osMutexRelease(xMonitorCacheMutexHandle);

    return modeChanged;
}

