/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "transport_uart.h"
#include "protocol.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for vWatchdogTask */
osThreadId_t vWatchdogTaskHandle;
const osThreadAttr_t vWatchdogTask_attributes = {
  .name = "vWatchdogTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh7,
};
/* Definitions for vInitTask */
osThreadId_t vInitTaskHandle;
const osThreadAttr_t vInitTask_attributes = {
  .name = "vInitTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh4,
};
/* Definitions for vObjectDetectio */
osThreadId_t vObjectDetectioHandle;
const osThreadAttr_t vObjectDetectio_attributes = {
  .name = "vObjectDetectio",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh2,
};
/* Definitions for vCommRxTask */
osThreadId_t vCommRxTaskHandle;
const osThreadAttr_t vCommRxTask_attributes = {
  .name = "vCommRxTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for vEventTask */
osThreadId_t vEventTaskHandle;
const osThreadAttr_t vEventTask_attributes = {
  .name = "vEventTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal5,
};
/* Definitions for vCommTxTask */
osThreadId_t vCommTxTaskHandle;
const osThreadAttr_t vCommTxTask_attributes = {
  .name = "vCommTxTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for vMonitorTask */
osThreadId_t vMonitorTaskHandle;
const osThreadAttr_t vMonitorTask_attributes = {
  .name = "vMonitorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
/* Definitions for phase3Echo */
osThreadId_t phase3EchoHandle;
const osThreadAttr_t phase3Echo_attributes = {
  .name = "phase3Echo",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for xEventQueue */
osMessageQueueId_t xEventQueueHandle;
const osMessageQueueAttr_t xEventQueue_attributes = {
  .name = "xEventQueue"
};
/* Definitions for xKeepAliveTxQueue */
osMessageQueueId_t xKeepAliveTxQueueHandle;
const osMessageQueueAttr_t xKeepAliveTxQueue_attributes = {
  .name = "xKeepAliveTxQueue"
};
/* Definitions for xEventTxQueue */
osMessageQueueId_t xEventTxQueueHandle;
const osMessageQueueAttr_t xEventTxQueue_attributes = {
  .name = "xEventTxQueue"
};
/* Definitions for xDataReportTxQueue */
osMessageQueueId_t xDataReportTxQueueHandle;
const osMessageQueueAttr_t xDataReportTxQueue_attributes = {
  .name = "xDataReportTxQueue"
};
/* Definitions for xConfigMutex */
osMutexId_t xConfigMutexHandle;
const osMutexAttr_t xConfigMutex_attributes = {
  .name = "xConfigMutex"
};
/* Definitions for xLogMutex */
osMutexId_t xLogMutexHandle;
const osMutexAttr_t xLogMutex_attributes = {
  .name = "xLogMutex"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void watchdogThread(void *argument);
void initThread(void *argument);
void objectDetectionThread(void *argument);
void CommRxThread(void *argument);
void eventThread(void *argument);
void commTxThread(void *argument);
void monitorThread(void *argument);
void Phase3EchoTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
//  uint8_t loopbackTestPassed = Phase1_UartLoopbackTest();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of xConfigMutex */
  xConfigMutexHandle = osMutexNew(&xConfigMutex_attributes);

  /* creation of xLogMutex */
  xLogMutexHandle = osMutexNew(&xLogMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of xEventQueue */
  xEventQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &xEventQueue_attributes);

  /* creation of xKeepAliveTxQueue */
  xKeepAliveTxQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &xKeepAliveTxQueue_attributes);

  /* creation of xEventTxQueue */
  xEventTxQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &xEventTxQueue_attributes);

  /* creation of xDataReportTxQueue */
  xDataReportTxQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &xDataReportTxQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of vWatchdogTask */
  vWatchdogTaskHandle = osThreadNew(watchdogThread, NULL, &vWatchdogTask_attributes);

  /* creation of vInitTask */
  vInitTaskHandle = osThreadNew(initThread, NULL, &vInitTask_attributes);

  /* creation of vObjectDetectio */
  vObjectDetectioHandle = osThreadNew(objectDetectionThread, NULL, &vObjectDetectio_attributes);

  /* creation of vCommRxTask */
  vCommRxTaskHandle = osThreadNew(CommRxThread, NULL, &vCommRxTask_attributes);

  /* creation of vEventTask */
  vEventTaskHandle = osThreadNew(eventThread, NULL, &vEventTask_attributes);

  /* creation of vCommTxTask */
  vCommTxTaskHandle = osThreadNew(commTxThread, NULL, &vCommTxTask_attributes);

  /* creation of vMonitorTask */
  vMonitorTaskHandle = osThreadNew(monitorThread, NULL, &vMonitorTask_attributes);

  /* creation of phase3Echo */
  phase3EchoHandle = osThreadNew(Phase3EchoTask, NULL, &phase3Echo_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_watchdogThread */
/**
  * @brief  Function implementing the vWatchdogTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_watchdogThread */
void watchdogThread(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_initThread */
/**
* @brief Function implementing the vInitTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_initThread */
void initThread(void *argument)
{
  /* USER CODE BEGIN initThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END initThread */
}

/* USER CODE BEGIN Header_objectDetectionThread */
/**
* @brief Function implementing the vObjectDetectio thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_objectDetectionThread */
void objectDetectionThread(void *argument)
{
  /* USER CODE BEGIN objectDetectionThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END objectDetectionThread */
}

/* USER CODE BEGIN Header_CommRxThread */
/**
* @brief Function implementing the vCommRxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CommRxThread */
void CommRxThread(void *argument)
{
  /* USER CODE BEGIN CommRxThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CommRxThread */
}

/* USER CODE BEGIN Header_eventThread */
/**
* @brief Function implementing the vEventTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_eventThread */
void eventThread(void *argument)
{
  /* USER CODE BEGIN eventThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END eventThread */
}

/* USER CODE BEGIN Header_commTxThread */
/**
* @brief Function implementing the vCommTxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_commTxThread */
void commTxThread(void *argument)
{
  /* USER CODE BEGIN commTxThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END commTxThread */
}

/* USER CODE BEGIN Header_monitorThread */
/**
* @brief Function implementing the vMonitorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_monitorThread */
void monitorThread(void *argument)
{
  /* USER CODE BEGIN monitorThread */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END monitorThread */
}

/* USER CODE BEGIN Header_Phase3EchoTask */
/**
* @brief Function implementing the phase3Echo thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Phase3EchoTask */
void Phase3EchoTask(void *argument)
{
  /* USER CODE BEGIN Phase3EchoTask */
  /* Infinite loop */
	 Uart_Init();
	  uint8_t rxBuf[64];
	  uint16_t rxLen = 0;

	  for (;;)
	  {
	    uint16_t n = Uart_Recv(rxBuf + rxLen, (uint16_t)(sizeof(rxBuf) - rxLen));
	    rxLen = (uint16_t)(rxLen + n);

	    if (rxLen > 0) {
	      uint8_t payload[64];
	      uint16_t payloadLen;
	      uint16_t consumed;
	      ProtoResult_t r = Frame_Decode(rxBuf, rxLen, payload, sizeof(payload), &payloadLen, &consumed);

	      if (r == PROTO_OK) {
	        uint8_t framed[150];
	        uint16_t framedLen;
	        Frame_Encode(payload, payloadLen, framed, sizeof(framed), &framedLen);
	        Uart_Send(framed, framedLen);

	        memmove(rxBuf, rxBuf + consumed, (size_t)(rxLen - consumed)); /* slide any leftover bytes to the front */
	        rxLen = (uint16_t)(rxLen - consumed);
	      } else if (r == PROTO_ERR_MALFORMED) {
	        memmove(rxBuf, rxBuf + consumed, (size_t)(rxLen - consumed)); /* discard the bad frame, resync */
	        rxLen = (uint16_t)(rxLen - consumed);
	      } else if (r == PROTO_ERR_BUFFER_TOO_SMALL) {
	        rxLen = 0; /* shouldn't happen given our buffer sizes; safe fallback if it ever does */
	      }
	      /* PROTO_ERR_INCOMPLETE: leave rxBuf as-is, wait for more bytes next loop */
	    }

	    osDelay(10);
	  }
  /* USER CODE END Phase3EchoTask */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
