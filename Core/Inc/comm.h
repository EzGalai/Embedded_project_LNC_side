/*
 * comm.h
 *
 *  Created on: Sep 14, 2026
 *      Author: ezgal
 */

#ifndef INC_COMM_H_
#define INC_COMM_H_

#include <stdint.h>

/**
 * @brief Enqueues an already-framed message onto the Keep-Alive priority
 * queue (highest priority — always drained first by vCommTxTask). Copies
 * framed into an internal pool slot, so the caller's buffer can go out of
 * scope immediately after this call returns.
 * @param framed Fully framed bytes (post Frame_Encode), ready to send as-is.
 * @param framedLen Length of framed.
 */
void Comm_SendKeepAlive(const uint8_t *framed, uint16_t framedLen);

/**
 * @brief Enqueues an already-framed message onto the Event priority queue
 * (drained after Keep-Alive, before Data Report). Same copy semantics as
 * Comm_SendKeepAlive.
 * @param framed Fully framed bytes (post Frame_Encode), ready to send as-is.
 * @param framedLen Length of framed.
 */
void Comm_SendEvent(const uint8_t *framed, uint16_t framedLen);

/**
 * @brief Enqueues an already-framed message onto the Data Report priority
 * queue (lowest priority — drained only when Keep-Alive and Event are both
 * empty). Same copy semantics as Comm_SendKeepAlive.
 * @param framed Fully framed bytes (post Frame_Encode), ready to send as-is.
 * @param framedLen Length of framed.
 */
void Comm_SendDataReport(const uint8_t *framed, uint16_t framedLen);

/**
 * @brief Body of vCommTxTask — an infinite loop draining the three TX
 * queues strictly by priority (Keep-Alive > Event > Data Report),
 * re-checking from the top after every send so a higher-priority message
 * can always cut in front of a backlog. Never returns.
 */
void Comm_RunTxTask(void);

#endif /* INC_COMM_H_ */
