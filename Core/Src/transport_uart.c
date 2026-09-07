#include "transport_uart.h"
#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart2;

#define UART_RX_BUFFER_SIZE 256

static volatile uint8_t rxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rxHead = 0;  /* next write index — advanced by the ISR */
static volatile uint16_t rxTail = 0;  /* next read index — advanced by Uart_Recv */
static uint8_t rxByte;                /* single-byte staging area for HAL_UART_Receive_IT */

void Uart_Init(void)
{
    rxHead = 0;
    rxTail = 0;
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}

void Uart_Send(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)data, len, HAL_MAX_DELAY);
}

uint16_t Uart_Recv(uint8_t *outBuf, uint16_t maxLen)
{
    uint16_t count = 0;
    while (count < maxLen && rxTail != rxHead) {
        outBuf[count++] = rxBuffer[rxTail];
        rxTail = (uint16_t)((rxTail + 1) % UART_RX_BUFFER_SIZE);
    }
    return count;
}

uint16_t Uart_Available(void)
{
    return (uint16_t)((rxHead - rxTail + UART_RX_BUFFER_SIZE) % UART_RX_BUFFER_SIZE);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uint16_t nextHead = (uint16_t)((rxHead + 1) % UART_RX_BUFFER_SIZE);
        if (nextHead != rxTail) {          /* drop the byte if the buffer is full, rather than overwrite */
            rxBuffer[rxHead] = rxByte;
            rxHead = nextHead;
        }
        HAL_UART_Receive_IT(huart, &rxByte, 1);  /* re-arm for the next byte */
    }
}
