/*
 * monitor.c — see monitor.h.
 */

#include "monitor.h"
#include "main.h"
#include "cmsis_os.h"
#include "dht11.h"
#include "adc_sensors.h"
#include "config.h"
#include <string.h>
#include<stdio.h>


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
	ConfigRecord_t config;
	Config_GetCurrent(&config);

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
    overall = WorstMode(overall, ClassifyRange(data.temperature, config.tempNormalLow, config.tempNormalHigh,
                                                config.tempWarningLow, config.tempWarningHigh));
    overall = WorstMode(overall, ClassifyMin(data.humidity, config.humidityNormalMin, config.humidityWarningMin));
    overall = WorstMode(overall, ClassifyMin(data.light, config.lightNormalMin, config.lightWarningMin));
    overall = WorstMode(overall, ClassifyMin(data.batteryVoltage, config.batteryNormalMin, config.batteryWarningMin));

    data.mode = overall;

    bool modeChanged = (previous.mode != data.mode);

    osMutexAcquire(xMonitorCacheMutexHandle, osWaitForever);
    memcpy(&g_latest, &data, sizeof(MonitorData_t));
    osMutexRelease(xMonitorCacheMutexHandle);

    return modeChanged;
}

