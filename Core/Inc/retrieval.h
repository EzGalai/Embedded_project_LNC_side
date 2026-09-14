/*
 * retrieval.h
 *
 *  Created on: Sep 14, 2026
 *      Author: ezgal
 */

#ifndef INC_RETRIEVAL_H_
#define INC_RETRIEVAL_H_

#include <stdint.h>

/**
 * @brief Handles a GET_MEASUREMENTS_REQ: reads matching lines from
 * /measurements/YYYY-MM-DD.log across the requested time range (resuming
 * from a CURSOR_DAY/CURSOR_OFFSET pair if present), packs as many
 * MEASUREMENT_RECORD entries as fit into one response, and sends
 * GET_MEASUREMENTS_RESP via Comm_SendDataReport — attaching a new cursor
 * if more data remains beyond this batch.
 * @param value Request's Value (TIME_RANGE_START/END, optional CURSOR_DAY/OFFSET).
 * @param valueLen Length of value.
 */
void Retrieval_HandleGetMeasurements(const uint8_t *value, uint16_t valueLen);

/**
 * @brief Handles a GET_EVENTS_REQ: reads matching lines from
 * /events/YYYY-MM-DD.log across the requested time range (resuming from a
 * CURSOR_DAY/CURSOR_OFFSET pair if present), packs as many EVENT_RECORD
 * entries as fit into one response — including a nested MEASUREMENT_RECORD
 * for Monitor-sourced events, matching what SendEventReport already sends
 * live — and sends GET_EVENTS_RESP via Comm_SendDataReport, attaching a new
 * cursor if more data remains beyond this batch.
 * @param value Request's Value (TIME_RANGE_START/END, optional CURSOR_DAY/OFFSET).
 * @param valueLen Length of value.
 */
void Retrieval_HandleGetEvents(const uint8_t *value, uint16_t valueLen);


#endif /* INC_RETRIEVAL_H_ */
