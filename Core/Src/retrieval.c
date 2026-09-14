/*
 * retrieval.c
 *
 *  Created on: Sep 14, 2026
 *      Author: ezgal
 */

/*
 * retrieval.c — see retrieval.h.
 */

#include "retrieval.h"
#include "protocol.h"
#include "comm.h"
#include "rtc_util.h"
#include "fatfs.h"
#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

extern osMutexId_t xLogMutexHandle;

/* Leaves headroom for STATUS + CURSOR fields + outer TAG wrapper + frame
   overhead within comm.c's 300-byte DataReport pool slot. */
#define RETRIEVAL_RESP_BUDGET 200
#define RETRIEVAL_LINE_MAX 96

/* Static, not stack-local — CommRxTask's stack is small, and FIL alone can
   be 500+ bytes depending on FatFS config. */
static uint8_t s_records[RETRIEVAL_RESP_BUDGET];
static uint8_t s_payload[280];
static uint8_t s_message[300];
static uint8_t s_framed[300];
static char s_line[RETRIEVAL_LINE_MAX];

static void SendMeasurementsResp(uint8_t status, uint16_t recordsLen,
                                  bool hasMore, uint32_t cursorDay, uint32_t cursorOffset)
{
    uint8_t valueBuf[4];
    uint16_t payloadLen = 0, written;

    valueBuf[0] = status;
    Protocol_EncodeTLV(PROTO_FIELD_STATUS, valueBuf, 1, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
    payloadLen = (uint16_t)(payloadLen + written);

    memcpy(s_payload + payloadLen, s_records, recordsLen);
    payloadLen = (uint16_t)(payloadLen + recordsLen);

    if (hasMore) {
        Protocol_PutU32(valueBuf, cursorDay);
        Protocol_EncodeTLV(PROTO_FIELD_CURSOR_DAY, valueBuf, 4, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);

        Protocol_PutU32(valueBuf, cursorOffset);
        Protocol_EncodeTLV(PROTO_FIELD_CURSOR_OFFSET, valueBuf, 4, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);
    }

    uint16_t messageLen;
    Protocol_EncodeTLV(PROTO_TAG_GET_MEASUREMENTS_RESP, s_payload, payloadLen, s_message, sizeof(s_message), &messageLen);

    uint16_t framedLen;
    Frame_Encode(s_message, messageLen, s_framed, sizeof(s_framed), &framedLen);
    Comm_SendDataReport(s_framed, framedLen);
}

void Retrieval_HandleGetMeasurements(const uint8_t *value, uint16_t valueLen)
{
    static FIL file;

    const uint8_t *field;
    uint16_t fieldLen;
    uint32_t rangeStart, rangeEnd;

    if (Protocol_FindField(value, valueLen, PROTO_FIELD_TIME_RANGE_START, &field, &fieldLen) != PROTO_OK || fieldLen < 4) {
        SendMeasurementsResp(PROTO_STATUS_INTERNAL_ERROR, 0, false, 0, 0);
        return;
    }
    Protocol_GetU32(field, &rangeStart);

    if (Protocol_FindField(value, valueLen, PROTO_FIELD_TIME_RANGE_END, &field, &fieldLen) != PROTO_OK || fieldLen < 4) {
        SendMeasurementsResp(PROTO_STATUS_INTERNAL_ERROR, 0, false, 0, 0);
        return;
    }
    Protocol_GetU32(field, &rangeEnd);

    if (rangeStart > rangeEnd) {
        SendMeasurementsResp(PROTO_STATUS_INVALID_TIME_RANGE, 0, false, 0, 0);
        return;
    }

    uint32_t resumeDay, resumeOffset;
    if (Protocol_FindField(value, valueLen, PROTO_FIELD_CURSOR_DAY, &field, &fieldLen) == PROTO_OK && fieldLen >= 4) {
        Protocol_GetU32(field, &resumeDay);
        if (Protocol_FindField(value, valueLen, PROTO_FIELD_CURSOR_OFFSET, &field, &fieldLen) == PROTO_OK && fieldLen >= 4) {
            Protocol_GetU32(field, &resumeOffset);
        } else {
            resumeOffset = 0;
        }
    } else {
        resumeDay = rangeStart / 86400u;
        resumeOffset = 0;
    }

    uint32_t endDay = rangeEnd / 86400u;

    uint16_t recordsLen = 0;
    uint16_t recordCount = 0;
    bool hasMore = false;
    uint32_t nextCursorDay = 0, nextCursorOffset = 0;

    osMutexAcquire(xLogMutexHandle, osWaitForever);

    for (uint32_t day = resumeDay; day <= endDay; day++) {
        char dateStr[11];
        RtcUtil_FormatDate(day * 86400u, dateStr);

        char path[32];
        snprintf(path, sizeof(path), "/measurements/%s.log", dateStr);

        if (f_open(&file, path, FA_READ) != FR_OK) {
            continue; /* no data logged this day — move on */
        }

        if (day == resumeDay && resumeOffset > 0) {
            f_lseek(&file, resumeOffset);
        }

        bool stopEntirely = false;

        for (;;) {
            uint32_t lineStart = (uint32_t)f_tell(&file);
            if (f_gets(s_line, sizeof(s_line), &file) == NULL) break; /* EOF this day */

            uint32_t ts; int temperature; unsigned humidity, light, battery, mode;
            if (sscanf(s_line, "%u,%d,%u,%u,%u,%u", &ts, &temperature, &humidity, &light, &battery, &mode) != 6) {
                continue; /* malformed/truncated line — skip it */
            }

            if (ts < rangeStart) continue;
            if (ts > rangeEnd) continue; /* out of range, but the log may contain multiple reboot-reset sessions — keep scanning rather than assume strict chronological order */


            uint8_t measurement[40]; uint16_t measurementLen = 0, mwritten;
            uint8_t valueBuf[4];

            Protocol_PutU32(valueBuf, ts);
            Protocol_EncodeTLV(PROTO_FIELD_TIMESTAMP, valueBuf, 4, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            Protocol_PutU16(valueBuf, (uint16_t)temperature);
            Protocol_EncodeTLV(PROTO_FIELD_TEMPERATURE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            valueBuf[0] = (uint8_t)humidity;
            Protocol_EncodeTLV(PROTO_FIELD_HUMIDITY, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            Protocol_PutU16(valueBuf, (uint16_t)light);
            Protocol_EncodeTLV(PROTO_FIELD_LIGHT, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            Protocol_PutU16(valueBuf, (uint16_t)battery);
            Protocol_EncodeTLV(PROTO_FIELD_BATTERY_VOLTAGE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            valueBuf[0] = (uint8_t)mode;
            Protocol_EncodeTLV(PROTO_FIELD_MODE, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
            measurementLen = (uint16_t)(measurementLen + mwritten);

            uint8_t recordBuf[40]; uint16_t recordLen;
            Protocol_EncodeTLV(PROTO_FIELD_MEASUREMENT_RECORD, measurement, measurementLen, recordBuf, sizeof(recordBuf), &recordLen);

            if ((uint16_t)(recordsLen + recordLen) > RETRIEVAL_RESP_BUDGET) {
                hasMore = true;
                nextCursorDay = day;
                nextCursorOffset = lineStart;
                stopEntirely = true;
                break;
            }

            memcpy(s_records + recordsLen, recordBuf, recordLen);
            recordsLen = (uint16_t)(recordsLen + recordLen);
            recordCount++;
        }

        f_close(&file);

        if (stopEntirely) break;
    }

    osMutexRelease(xLogMutexHandle);

    uint8_t status = (recordCount == 0 && !hasMore) ? PROTO_STATUS_NO_DATA_FOUND : PROTO_STATUS_SUCCESS;
    SendMeasurementsResp(status, recordsLen, hasMore, nextCursorDay, nextCursorOffset);
}


static void SendEventsResp(uint8_t status, uint16_t recordsLen,
                            bool hasMore, uint32_t cursorDay, uint32_t cursorOffset)
{
    uint8_t valueBuf[4];
    uint16_t payloadLen = 0, written;

    valueBuf[0] = status;
    Protocol_EncodeTLV(PROTO_FIELD_STATUS, valueBuf, 1, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
    payloadLen = (uint16_t)(payloadLen + written);

    memcpy(s_payload + payloadLen, s_records, recordsLen);
    payloadLen = (uint16_t)(payloadLen + recordsLen);

    if (hasMore) {
        Protocol_PutU32(valueBuf, cursorDay);
        Protocol_EncodeTLV(PROTO_FIELD_CURSOR_DAY, valueBuf, 4, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);

        Protocol_PutU32(valueBuf, cursorOffset);
        Protocol_EncodeTLV(PROTO_FIELD_CURSOR_OFFSET, valueBuf, 4, s_payload + payloadLen, (uint16_t)(sizeof(s_payload) - payloadLen), &written);
        payloadLen = (uint16_t)(payloadLen + written);
    }

    uint16_t messageLen;
    Protocol_EncodeTLV(PROTO_TAG_GET_EVENTS_RESP, s_payload, payloadLen, s_message, sizeof(s_message), &messageLen);

    uint16_t framedLen;
    Frame_Encode(s_message, messageLen, s_framed, sizeof(s_framed), &framedLen);
    Comm_SendDataReport(s_framed, framedLen);
}

void Retrieval_HandleGetEvents(const uint8_t *value, uint16_t valueLen)
{
    static FIL file;

    const uint8_t *field;
    uint16_t fieldLen;
    uint32_t rangeStart, rangeEnd;

    if (Protocol_FindField(value, valueLen, PROTO_FIELD_TIME_RANGE_START, &field, &fieldLen) != PROTO_OK || fieldLen < 4) {
        SendEventsResp(PROTO_STATUS_INTERNAL_ERROR, 0, false, 0, 0);
        return;
    }
    Protocol_GetU32(field, &rangeStart);

    if (Protocol_FindField(value, valueLen, PROTO_FIELD_TIME_RANGE_END, &field, &fieldLen) != PROTO_OK || fieldLen < 4) {
        SendEventsResp(PROTO_STATUS_INTERNAL_ERROR, 0, false, 0, 0);
        return;
    }
    Protocol_GetU32(field, &rangeEnd);

    if (rangeStart > rangeEnd) {
        SendEventsResp(PROTO_STATUS_INVALID_TIME_RANGE, 0, false, 0, 0);
        return;
    }

    uint32_t resumeDay, resumeOffset;
    if (Protocol_FindField(value, valueLen, PROTO_FIELD_CURSOR_DAY, &field, &fieldLen) == PROTO_OK && fieldLen >= 4) {
        Protocol_GetU32(field, &resumeDay);
        if (Protocol_FindField(value, valueLen, PROTO_FIELD_CURSOR_OFFSET, &field, &fieldLen) == PROTO_OK && fieldLen >= 4) {
            Protocol_GetU32(field, &resumeOffset);
        } else {
            resumeOffset = 0;
        }
    } else {
        resumeDay = rangeStart / 86400u;
        resumeOffset = 0;
    }

    uint32_t endDay = rangeEnd / 86400u;

    uint16_t recordsLen = 0;
    uint16_t recordCount = 0;
    bool hasMore = false;
    uint32_t nextCursorDay = 0, nextCursorOffset = 0;

    osMutexAcquire(xLogMutexHandle, osWaitForever);

    for (uint32_t day = resumeDay; day <= endDay; day++) {
        char dateStr[11];
        RtcUtil_FormatDate(day * 86400u, dateStr);

        char path[32];
        snprintf(path, sizeof(path), "/events/%s.log", dateStr);

        if (f_open(&file, path, FA_READ) != FR_OK) {
            continue; /* no events logged this day — move on */
        }

        if (day == resumeDay && resumeOffset > 0) {
            f_lseek(&file, resumeOffset);
        }

        bool stopEntirely = false;

        for (;;) {
            uint32_t lineStart = (uint32_t)f_tell(&file);
            if (f_gets(s_line, sizeof(s_line), &file) == NULL) break; /* EOF this day */

            /* Event lines come in two shapes (see log.c's Log_WriteEvent):
               "ts,type,source" (Object Detection) or
               "ts,type,source,temp,humidity,light,battery,mode" (Monitor).
               Try the longer shape first — sscanf just stops matching and
               returns how many conversions actually succeeded. */
            uint32_t ts; unsigned eventType, eventSource;
            int temperature; unsigned humidity, light, battery, mode;
            int scanned = sscanf(s_line, "%u,%u,%u,%d,%u,%u,%u,%u",
                                  &ts, &eventType, &eventSource, &temperature, &humidity, &light, &battery, &mode);
            bool hasMeasurement = (scanned == 8);
            if (!hasMeasurement && scanned != 3) {
                continue; /* malformed/truncated line — skip it */
            }

            if (ts < rangeStart) continue;
            if (ts > rangeEnd) continue; /* out of range, but the log may contain multiple reboot-reset sessions — keep scanning rather than assume strict chronological order */


            uint8_t eventBuf[80]; uint16_t eventLen = 0, ewritten;
            uint8_t valueBuf[4];

            Protocol_PutU32(valueBuf, ts);
            Protocol_EncodeTLV(PROTO_FIELD_TIMESTAMP, valueBuf, 4, eventBuf + eventLen, (uint16_t)(sizeof(eventBuf) - eventLen), &ewritten);
            eventLen = (uint16_t)(eventLen + ewritten);

            valueBuf[0] = (uint8_t)eventType;
            Protocol_EncodeTLV(PROTO_FIELD_EVENT_TYPE, valueBuf, 1, eventBuf + eventLen, (uint16_t)(sizeof(eventBuf) - eventLen), &ewritten);
            eventLen = (uint16_t)(eventLen + ewritten);

            valueBuf[0] = (uint8_t)eventSource;
            Protocol_EncodeTLV(PROTO_FIELD_EVENT_SOURCE, valueBuf, 1, eventBuf + eventLen, (uint16_t)(sizeof(eventBuf) - eventLen), &ewritten);
            eventLen = (uint16_t)(eventLen + ewritten);

            if (hasMeasurement) {
                uint8_t measurement[40]; uint16_t measurementLen = 0, mwritten;

                Protocol_PutU32(valueBuf, ts);
                Protocol_EncodeTLV(PROTO_FIELD_TIMESTAMP, valueBuf, 4, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                Protocol_PutU16(valueBuf, (uint16_t)temperature);
                Protocol_EncodeTLV(PROTO_FIELD_TEMPERATURE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                valueBuf[0] = (uint8_t)humidity;
                Protocol_EncodeTLV(PROTO_FIELD_HUMIDITY, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                Protocol_PutU16(valueBuf, (uint16_t)light);
                Protocol_EncodeTLV(PROTO_FIELD_LIGHT, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                Protocol_PutU16(valueBuf, (uint16_t)battery);
                Protocol_EncodeTLV(PROTO_FIELD_BATTERY_VOLTAGE, valueBuf, 2, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                valueBuf[0] = (uint8_t)mode;
                Protocol_EncodeTLV(PROTO_FIELD_MODE, valueBuf, 1, measurement + measurementLen, (uint16_t)(sizeof(measurement) - measurementLen), &mwritten);
                measurementLen = (uint16_t)(measurementLen + mwritten);

                Protocol_EncodeTLV(PROTO_FIELD_MEASUREMENT_RECORD, measurement, measurementLen, eventBuf + eventLen, (uint16_t)(sizeof(eventBuf) - eventLen), &ewritten);
                eventLen = (uint16_t)(eventLen + ewritten);
            }

            uint8_t recordBuf[80]; uint16_t recordLen;
            Protocol_EncodeTLV(PROTO_FIELD_EVENT_RECORD, eventBuf, eventLen, recordBuf, sizeof(recordBuf), &recordLen);

            if ((uint16_t)(recordsLen + recordLen) > RETRIEVAL_RESP_BUDGET) {
                hasMore = true;
                nextCursorDay = day;
                nextCursorOffset = lineStart;
                stopEntirely = true;
                break;
            }

            memcpy(s_records + recordsLen, recordBuf, recordLen);
            recordsLen = (uint16_t)(recordsLen + recordLen);
            recordCount++;
        }

        f_close(&file);

        if (stopEntirely) break;
    }

    osMutexRelease(xLogMutexHandle);

    uint8_t status = (recordCount == 0 && !hasMore) ? PROTO_STATUS_NO_DATA_FOUND : PROTO_STATUS_SUCCESS;
    SendEventsResp(status, recordsLen, hasMore, nextCursorDay, nextCursorOffset);
}



