/*
 * transport_uart_backed.c
 *
 *  Created on: Sep 7, 2026
 *      Author: ezgal
 */
#include "transport_uart.h"
#include "transport.h"



void Transport_Init(void){
	Uart_Init();
}


void Transport_Send(const uint8_t *data, uint16_t len){
	Uart_Send(data, len);
}


uint16_t Transport_Recv(uint8_t *outBuf, uint16_t maxLen){
	return Uart_Recv(outBuf, maxLen);
}






