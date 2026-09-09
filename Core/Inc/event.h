/*
 * event.h — Event module (PROJECT_PLAN.md §4.4). Defines the event message
 * shape and the pool-based posting mechanism used to get an event from
 * vMonitorTask (or later, Object Detection/Configuration/Init) to
 * vEventTask via xEventQueue, without needing the queue's item size to
 * track EventMessage_t's size.
 */

#ifndef INC_EVENT_H_
#define INC_EVENT_H_

#include "protocol.h"
#include "monitor.h"

typedef struct {
    ProtoEventSource_t source;
    ProtoEventType_t type;
    ProtoMode_t mode;
    MonitorData_t measurement;
} EventMessage_t;

/**
 * @brief Posts a Monitor mode-change event: writes it into the next pool
 * slot (of EVENT_POOL_SIZE, matching xEventQueue's depth so a slot can
 * never be overwritten while still unread) and enqueues that slot's index
 * onto xEventQueue. Called by vMonitorTask when Monitor_Sample() reports a
 * mode change.
 * @param mode New mode.
 * @param measurement Snapshot of the measurement that produced this mode.
 */
void Event_PostModeChange(ProtoMode_t mode, const MonitorData_t *measurement);

/**
 * @brief Retrieves a previously-posted event by its pool slot index.
 * @param slotIndex Index received from xEventQueue.
 * @param outEvent Set to a copy of the pooled event.
 */
void Event_GetPooled(uint16_t slotIndex, EventMessage_t *outEvent);

/**
 * @brief Drives the RGB LED and buzzer for a given mode's event, and sends
 * an EVENT_REPORT for it.
 * @param event The event to handle.
 */
void Event_HandleModeChange(const EventMessage_t *event);

#endif /* INC_EVENT_H_ */
