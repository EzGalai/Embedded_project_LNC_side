/*
 * monitor.h — Monitor module (PROJECT_PLAN.md §4.2). Holds the shared cache
 * of the latest sensor reading + computed Mode, written by vMonitorTask once
 * per 5s cycle and read by vKeepAliveTask and vEventTask. Protected by
 * xMonitorCacheMutex since a torn read/write across a multi-field struct
 * could otherwise mix stale and fresh values.
 */

#ifndef INC_MONITOR_H_
#define INC_MONITOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h" /* for ProtoMode_t */

typedef struct {
    uint32_t timestamp;
    int16_t  temperature;      /* tenths of degC, matches PROTO_FIELD_TEMPERATURE */
    uint8_t  humidity;         /* %, matches PROTO_FIELD_HUMIDITY */
    uint16_t light;            /* raw ADC (0-4095), matches PROTO_FIELD_LIGHT */
    uint16_t batteryVoltage;   /* mV, matches PROTO_FIELD_BATTERY_VOLTAGE */
    ProtoMode_t mode;
} MonitorData_t;

/**
 * @brief Thread-safe read of the latest cached measurement + mode.
 * @param outData Set to a copy of the current cache.
 */
void Monitor_GetLatest(MonitorData_t *outData);

/**
 * @brief Thread-safe write of a new measurement + mode into the cache.
 * Called only by vMonitorTask, once per 5s sampling cycle.
 * @param data New values to store.
 */
void Monitor_SetLatest(const MonitorData_t *data);

void Monitor_GetLatest(MonitorData_t *outData);
/**
 * @brief Samples DHT11 (temp/humidity) and ADC (battery/light), classifies
 * each against the Normal/Warning/Error thresholds (PROJECT_PLAN.md's
 * Operating Modes table), computes the overall Mode, and updates the shared
 * cache. Called once per vMonitorTask cycle (every 5s).
 * @return true if the overall Mode changed since the previous sample.
 */
bool Monitor_Sample(void);

#endif /* INC_MONITOR_H_ */
