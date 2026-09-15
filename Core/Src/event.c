/*
 * event.c — see event.h.
 */

#include "event.h"
#include "main.h"      /* xEventQueueHandle, RGB_*_Pin, Buzzer_Pin */
#include "cmsis_os.h"
#include "transport.h"
#include "rtc_util.h"
#include "comm.h"

#include <string.h>

#define EVENT_POOL_SIZE 16  /* matches xEventQueue's depth (osMessageQueueNew(16, ...)) */

static EventMessage_t g_eventPool[EVENT_POOL_SIZE];
static uint16_t g_eventPoolNext = 0;
static bool g_objectDetected = false;
static ProtoMode_t g_lastMonitorMode = PROTO_MODE_NORMAL;

static void SetRgb(GPIO_PinState red, GPIO_PinState green, GPIO_PinState blue)
{
    HAL_GPIO_WritePin(RGB_RED_GPIO_Port, RGB_RED_Pin, red);
    HAL_GPIO_WritePin(RGB_GREEN_GPIO_Port, RGB_GREEN_Pin, green);
    HAL_GPIO_WritePin(RGB_BLUE_GPIO_Port, RGB_BLUE_Pin, blue);
}

static void ApplyLedForMode(ProtoMode_t mode)
{
    switch (mode) {
        case PROTO_MODE_NORMAL:  SetRgb(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_RESET); break;
        case PROTO_MODE_WARNING: SetRgb(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET); break;
        case PROTO_MODE_ERROR:   SetRgb(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET); break;
    }
}


/* Recomputes the actual LED/alarm state from both inputs — Object Detection
   wins outright while active; once cleared, falls back to Monitor's last mode. */
static void RefreshLedAndAlarm(void)
{
    if (g_objectDetected) {
        SetRgb(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET); /* red */
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    } else {
        ApplyLedForMode(g_lastMonitorMode);
        if (g_lastMonitorMode == PROTO_MODE_ERROR) {
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
        } else {
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        }
    }
}


static void SendEventReport(const EventMessage_t *event)
{
    uint8_t valueBuf[4]; uint16_t written;
    uint8_t payload[96]; uint16_t payloadLen = 0;

    Protocol_PutU32(valueBuf, event->measurement.timestamp);
    Protocol_EncodeTLV(PROTO_FIELD_TIMESTAMP, valueBuf, 4, payload + payloadLen, (uint16_t)(sizeof(payload) - payloadLen), &written);
    payloadLen = (uint16_t)(payloadLen + written);

    valueBuf[0] = (uint8_t)event->type;
    Protocol_EncodeTLV(PROTO_FIELD_EVENT_TYPE, valueBuf, 1, payload + payloadLen, (uint16_t)(sizeof(payload) - payloadLen), &written);
    payloadLen = (uint16_t)(payloadLen + written);

    valueBuf[0] = (uint8_t)event->source;
    Protocol_EncodeTLV(PROTO_FIELD_EVENT_SOURCE, valueBuf, 1, payload + payloadLen, (uint16_t)(sizeof(payload) - payloadLen), &written);
    payloadLen = (uint16_t)(payloadLen + written);

    if (event->source == PROTO_EVENT_SOURCE_MONITOR) {
        uint8_t measurement[40]; uint16_t measurementLen = 0;

        Protocol_PutU32(valueBuf, event->measurement.timestamp);
        Protocol_EncodeTLV(PROTO_FIELD_TIMESTAMP, valueBuf, 4, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        Protocol_PutU16(valueBuf, (uint16_t)event->measurement.temperature);
        Protocol_EncodeTLV(PROTO_FIELD_TEMPERATURE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        valueBuf[0] = event->measurement.humidity;
        Protocol_EncodeTLV(PROTO_FIELD_HUMIDITY, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        Protocol_PutU16(valueBuf, event->measurement.light);
        Protocol_EncodeTLV(PROTO_FIELD_LIGHT, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        Protocol_PutU16(valueBuf, event->measurement.batteryVoltage);
        Protocol_EncodeTLV(PROTO_FIELD_BATTERY_VOLTAGE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        valueBuf[0] = (uint8_t)event->measurement.mode;
        Protocol_EncodeTLV(PROTO_FIELD_MODE, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &written);
        measurementLen = (uint16_t)(measurementLen + written);

        Protocol_EncodeTLV(PROTO_FIELD_MEASUREMENT_RECORD, measurement, measurementLen, payload + payloadLen, (uint16_t)(sizeof(payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);

    }else if (event->type == PROTO_EVENT_TYPE_STARTUP) {
        valueBuf[0] = (uint8_t)event->wdResetFlag;
        Protocol_EncodeTLV(PROTO_FIELD_WD_RESET_FLAG, valueBuf, 1, payload + payloadLen, (uint16_t)(sizeof(payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);
    }

    uint8_t message[120]; uint16_t messageLen;
    Protocol_EncodeTLV(PROTO_TAG_EVENT_REPORT, payload, payloadLen, message, sizeof(message), &messageLen);
    uint8_t framed[300]; uint16_t framedLen;
    Frame_Encode(message, messageLen, framed, sizeof(framed), &framedLen);
    Comm_SendEvent(framed, framedLen);

}



void Event_PostModeChange(ProtoMode_t mode, const MonitorData_t *measurement)
{
    uint16_t slot = g_eventPoolNext;
    g_eventPoolNext = (uint16_t)((g_eventPoolNext + 1) % EVENT_POOL_SIZE);

    g_eventPool[slot].source = PROTO_EVENT_SOURCE_MONITOR;
    g_eventPool[slot].type = PROTO_EVENT_TYPE_MODE_CHANGE;
    g_eventPool[slot].mode = mode;
    memcpy(&g_eventPool[slot].measurement, measurement, sizeof(MonitorData_t));


    osMessageQueuePut(xEventQueueHandle, &slot, 0, 0);
}

void Event_GetPooled(uint16_t slotIndex, EventMessage_t *outEvent)
{
    memcpy(outEvent, &g_eventPool[slotIndex], sizeof(EventMessage_t));
}

void Event_HandleModeChange(const EventMessage_t *event)
{
    g_lastMonitorMode = event->mode;
    RefreshLedAndAlarm();
    SendEventReport(event);
}


void Event_PostObjectDetection(bool detected)
{
    uint16_t slot = g_eventPoolNext;
    g_eventPoolNext = (uint16_t)((g_eventPoolNext + 1) % EVENT_POOL_SIZE);

    g_eventPool[slot].source = PROTO_EVENT_SOURCE_OBJECT_DETECTION;
    g_eventPool[slot].type = detected ? PROTO_EVENT_TYPE_OBJECT_DETECTED : PROTO_EVENT_TYPE_OBJECT_CLEARED;
    g_eventPool[slot].mode = PROTO_MODE_NORMAL;
    memset(&g_eventPool[slot].measurement, 0, sizeof(MonitorData_t));
    g_eventPool[slot].measurement.timestamp = RtcUtil_GetUnixTime();

    osMessageQueuePut(xEventQueueHandle, &slot, 0, 0);
}



void Event_HandleObjectDetection(const EventMessage_t *event)
{
    g_objectDetected = (event->type == PROTO_EVENT_TYPE_OBJECT_DETECTED);
    RefreshLedAndAlarm();
    SendEventReport(event);
}

void Event_PostStartup(bool wasWatchdogReset)
{
    uint16_t slot = g_eventPoolNext;
    g_eventPoolNext = (uint16_t)((g_eventPoolNext + 1) % EVENT_POOL_SIZE);

    g_eventPool[slot].source = PROTO_EVENT_SOURCE_INIT;
    g_eventPool[slot].type = PROTO_EVENT_TYPE_STARTUP;
    g_eventPool[slot].mode = PROTO_MODE_NORMAL;
    memset(&g_eventPool[slot].measurement, 0, sizeof(MonitorData_t));
    g_eventPool[slot].measurement.timestamp = RtcUtil_GetUnixTime();
    g_eventPool[slot].wdResetFlag = wasWatchdogReset;

    osMessageQueuePut(xEventQueueHandle, &slot, 0, 0);
}

void Event_HandleStartup(const EventMessage_t *event)
{
    SendEventReport(event);
}


