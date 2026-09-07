/*
 * transport_uart.h
 *
 *  Created on: Sep 6, 2026
 *      Author: ezgal
 */

#ifndef INC_TRANSPORT_UART_H_
#define INC_TRANSPORT_UART_H_

#include <stdint.h>

/**
 * @brief Initialize the UART peripheral and start interrupt-driven reception.
 *
 * Must be called once, before any other Uart_* function. From this point on,
 * incoming bytes are captured into an internal ring buffer automatically —
 * no further action is needed to keep receiving.
 */
void Uart_Init(void);

/**
 * @brief Send bytes over UART, blocking until the transmission completes.
 * @param data Bytes to send.
 * @param len  Number of bytes to send.
 */
void Uart_Send(const uint8_t *data, uint16_t len);

/**
 * @brief Copy bytes already captured in the RX ring buffer, without blocking.
 * @param outBuf Destination buffer for the copied bytes.
 * @param maxLen Maximum number of bytes outBuf can hold.
 * @return Number of bytes actually copied; 0 if nothing has been received yet.
 */
uint16_t Uart_Recv(uint8_t *outBuf, uint16_t maxLen);

/**
 * @brief Report how many bytes are currently waiting, unread, in the RX ring buffer.
 * @return Number of bytes available to read.
 */
uint16_t Uart_Available(void);

#endif /* INC_TRANSPORT_UART_H_ */
