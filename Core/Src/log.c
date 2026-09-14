/*
 * log.c
 *
 *  Created on: Sep 14, 2026
 *      Author: ezgal
 */

#include "log.h"
#include "rtc_util.h"
#include "fatfs.h"
#include "cmsis_os.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

#define LOG_MAX_FILES 7
#define LOG_DATE_LEN 10 /* "YYYY-MM-DD" */
extern osMutexId_t xLogMutexHandle;


static char g_lastLogDate[LOG_DATE_LEN + 1] = "";

/**
 * @brief Deletes the oldest date-named ("YYYY-MM-DD.log") file in dirPath
 * if more than LOG_MAX_FILES exist there. Any FatFS failure (missing SD
 * card, bad directory, etc.) is treated as "nothing to rotate" and
 * silently ignored.
 */
static void RotateDirIfNeeded(const char *dirPath)
{
    DIR dir;
    FILINFO fno;
    char oldest[LOG_DATE_LEN + 5] = ""; /* "YYYY-MM-DD.log" + null */
    int count = 0;

    if (f_opendir(&dir, dirPath) != FR_OK) return;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
        if (fno.fattrib & AM_DIR) continue;
        if (strlen(fno.fname) != LOG_DATE_LEN + 4) continue; /* "YYYY-MM-DD.log" = 14 chars */
        count++;
        if (oldest[0] == '\0' || strcmp(fno.fname, oldest) < 0) {
            strncpy(oldest, fno.fname, sizeof(oldest) - 1);
        }
    }
    f_closedir(&dir);

    if (count > LOG_MAX_FILES && oldest[0] != '\0') {
        char fullPath[32];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, oldest);
        f_unlink(fullPath);
    }
}

/**
 * @brief Rotates both log folders if todayStr differs from the last date a
 * write was made under — i.e., once, on the first write of a new day.
 */
static void RotateIfNewDay(const char *todayStr)
{
    if (strcmp(g_lastLogDate, todayStr) == 0) return;

    RotateDirIfNeeded("/measurements");
    RotateDirIfNeeded("/events");

    strncpy(g_lastLogDate, todayStr, sizeof(g_lastLogDate) - 1);
}

void Log_Init(void)
{
    f_mkdir("/measurements"); /* FR_EXIST (or any other failure) is fine to ignore here */
    f_mkdir("/events");
}

void Log_WriteMeasurement(const MonitorData_t *data)
{
    char today[LOG_DATE_LEN + 1];
    RtcUtil_GetTodayString(today);

    osMutexAcquire(xLogMutexHandle, osWaitForever);
    RotateIfNewDay(today);

    char path[32];
    snprintf(path, sizeof(path), "/measurements/%s.log", today);

    FIL file;
    if (f_open(&file, path, FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
        char line[80];
        int len = snprintf(line, sizeof(line), "%u,%d,%u,%u,%u,%u\r\n",
                            (unsigned)data->timestamp, data->temperature,
                            data->humidity, data->light, data->batteryVoltage,
                            (unsigned)data->mode);
        UINT written;
        f_write(&file, line, (UINT)len, &written);
        f_close(&file);
    }

    osMutexRelease(xLogMutexHandle);
}

void Log_WriteEvent(const EventMessage_t *event)
{
    char today[LOG_DATE_LEN + 1];
    RtcUtil_GetTodayString(today);

    osMutexAcquire(xLogMutexHandle, osWaitForever);
    RotateIfNewDay(today);

    char path[32];
    snprintf(path, sizeof(path), "/events/%s.log", today);

    FIL file;
    if (f_open(&file, path, FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
        char line[96];
        int len;
        if (event->source == PROTO_EVENT_SOURCE_MONITOR) {
            len = snprintf(line, sizeof(line), "%u,%u,%u,%d,%u,%u,%u,%u\r\n",
                            (unsigned)event->measurement.timestamp,
                            (unsigned)event->type, (unsigned)event->source,
                            event->measurement.temperature, event->measurement.humidity,
                            event->measurement.light, event->measurement.batteryVoltage,
                            (unsigned)event->measurement.mode);
        } else {
            len = snprintf(line, sizeof(line), "%u,%u,%u\r\n",
                            (unsigned)event->measurement.timestamp,
                            (unsigned)event->type, (unsigned)event->source);
        }
        UINT written;
        f_write(&file, line, (UINT)len, &written);
        f_close(&file);
    }

    osMutexRelease(xLogMutexHandle);
}

