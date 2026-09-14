#ifndef INC_LOG_H_
#define INC_LOG_H_
#include "monitor.h"
#include "event.h"

/**
 * @brief Ensures /measurements and /events exist on the SD card. Call once
 * at boot, right after f_mount(). Safe to call even if the SD card failed
 * to mount or these directories already exist — errors are ignored, since
 * a missing/failed SD card should degrade logging only, not the rest of
 * the system.
 */
void Log_Init(void);

/**
 * @brief Appends one line to today's measurement log
 * (/measurements/YYYY-MM-DD.log): timestamp,temperature,humidity,light,
 * battery,mode. Rotates both log folders (deletes the oldest file if more
 * than 7 exist) on the first write of a new day. Fails silently (skips the
 * write) if the SD card is unavailable — logging is a best-effort
 * convenience, never a dependency of core sensing/alarm/comms functions.
 * @param data Latest sample from Monitor.
 */
void Log_WriteMeasurement(const MonitorData_t *data);

/**
 * @brief Appends one line to today's event log (/events/YYYY-MM-DD.log):
 * timestamp,eventType,eventSource, plus (for Monitor-sourced events only)
 * the same measurement fields as Log_WriteMeasurement. Rotates the same
 * way as Log_WriteMeasurement.
 * @param event Event being logged.
 */
void Log_WriteEvent(const EventMessage_t *event);


#endif /* INC_LOG_H_ */
