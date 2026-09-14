/*
 * comm.c
 *
 *  Created on: Sep 14, 2026
 *      Author: ezgal
 */


#include "comm.h"
#include "main.h"
#include "cmsis_os.h"
#include "transport.h"
#include <string.h>

extern osMessageQueueId_t xKeepAliveTxQueueHandle;
extern osMessageQueueId_t xEventTxQueueHandle;
extern osMessageQueueId_t xDataReportTxQueueHandle;

#define COMM_QUEUE_DEPTH 16 /* must match each queue's osMessageQueueNew depth */

#define KEEPALIVE_MAX_LEN   200
#define EVENT_MAX_LEN       300
#define DATAREPORT_MAX_LEN  300

typedef struct { uint8_t framed[KEEPALIVE_MAX_LEN];  uint16_t framedLen; } KeepAlivePoolEntry_t;
typedef struct { uint8_t framed[EVENT_MAX_LEN];      uint16_t framedLen; } EventPoolEntry_t;
typedef struct { uint8_t framed[DATAREPORT_MAX_LEN]; uint16_t framedLen; } DataReportPoolEntry_t;

static KeepAlivePoolEntry_t g_keepAlivePool[COMM_QUEUE_DEPTH];
static uint16_t g_keepAlivePoolNext = 0;

static EventPoolEntry_t g_eventTxPool[COMM_QUEUE_DEPTH];
static uint16_t g_eventTxPoolNext = 0;

static DataReportPoolEntry_t g_dataReportPool[COMM_QUEUE_DEPTH];
static uint16_t g_dataReportPoolNext = 0;

void Comm_SendKeepAlive(const uint8_t *framed, uint16_t framedLen)
{
    if (framedLen > KEEPALIVE_MAX_LEN) return; /* refuse to enqueue something we can't safely copy */

    uint16_t slot = g_keepAlivePoolNext;
    g_keepAlivePoolNext = (uint16_t)((g_keepAlivePoolNext + 1) % COMM_QUEUE_DEPTH);

    memcpy(g_keepAlivePool[slot].framed, framed, framedLen);
    g_keepAlivePool[slot].framedLen = framedLen;

    osMessageQueuePut(xKeepAliveTxQueueHandle, &slot, 0, 0);
}

void Comm_SendEvent(const uint8_t *framed, uint16_t framedLen)
{
    if (framedLen > EVENT_MAX_LEN) return;

    uint16_t slot = g_eventTxPoolNext;
    g_eventTxPoolNext = (uint16_t)((g_eventTxPoolNext + 1) % COMM_QUEUE_DEPTH);

    memcpy(g_eventTxPool[slot].framed, framed, framedLen);
    g_eventTxPool[slot].framedLen = framedLen;

    osMessageQueuePut(xEventTxQueueHandle, &slot, 0, 0);
}

void Comm_SendDataReport(const uint8_t *framed, uint16_t framedLen)
{
    if (framedLen > DATAREPORT_MAX_LEN) return;

    uint16_t slot = g_dataReportPoolNext;
    g_dataReportPoolNext = (uint16_t)((g_dataReportPoolNext + 1) % COMM_QUEUE_DEPTH);

    memcpy(g_dataReportPool[slot].framed, framed, framedLen);
    g_dataReportPool[slot].framedLen = framedLen;

    osMessageQueuePut(xDataReportTxQueueHandle, &slot, 0, 0);
}

void Comm_RunTxTask(void)
{
    for (;;) {
        uint16_t slot;

        if (osMessageQueueGet(xKeepAliveTxQueueHandle, &slot, NULL, 0) == osOK) {
            Transport_Send(g_keepAlivePool[slot].framed, g_keepAlivePool[slot].framedLen);
            continue;
        }
        if (osMessageQueueGet(xEventTxQueueHandle, &slot, NULL, 0) == osOK) {
            Transport_Send(g_eventTxPool[slot].framed, g_eventTxPool[slot].framedLen);
            continue;
        }
        if (osMessageQueueGet(xDataReportTxQueueHandle, &slot, NULL, 5) == osOK) {
            Transport_Send(g_dataReportPool[slot].framed, g_dataReportPool[slot].framedLen);
            continue;
        }
        /* all three empty even after a short wait — loop back and recheck from the top */
    }
}
