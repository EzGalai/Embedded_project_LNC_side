///*
// * dht11.c
// *
// *  Created on: Sep 6, 2026
// *      Author: ezgal
// */
//
//
//#include "dht11.h"
//
//static void delay_us(uint32_t us, TIM_HandleTypeDef *htim) {
//    __HAL_TIM_SET_COUNTER(htim, 0);
//    while (__HAL_TIM_GET_COUNTER(htim) < us);
//}
//
//static void switch_to_output(uint16_t gpio_pin, GPIO_TypeDef  *GPIOx){
//	GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//	HAL_GPIO_WritePin(GPIOx, gpio_pin, GPIO_PIN_SET);
//
//	 GPIO_InitStruct.Pin = gpio_pin;
//	 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//	 HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
//}
//
//static void switch_to_input(uint16_t gpio_pin, GPIO_TypeDef  *GPIOx){
//	GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//
//	 GPIO_InitStruct.Pin = gpio_pin;
//	 GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//	 GPIO_InitStruct.Pull = GPIO_PULLUP;
//	 HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
//}
//
//
//
//static int wait_for_level_or_timeout(uint16_t gpio_pin, GPIO_TypeDef *GPIOx, GPIO_PinState level,
//                                       TIM_HandleTypeDef *htim, uint32_t max_time){
//    uint16_t start_time = __HAL_TIM_GET_COUNTER(htim);
//    uint16_t elapsed_time = 0;
//
//    while(HAL_GPIO_ReadPin(GPIOx, gpio_pin) != level){
//        elapsed_time = (uint16_t)(__HAL_TIM_GET_COUNTER(htim) - start_time);
//        if(elapsed_time >= max_time){
//            return -1;
//        }
//    }
//
//    uint16_t final_elapsed_time = (uint16_t)(__HAL_TIM_GET_COUNTER(htim) - start_time);
//
//    return (int)final_elapsed_time;
//}
//
//
//
//static int read_bit(uint16_t gpio_pin, GPIO_TypeDef *GPIOx, TIM_HandleTypeDef *htim, int* bit_val){
//    // Wait for the line to go low — could still be finishing the ack's high phase (~80us) at this point
//    if (wait_for_level_or_timeout(gpio_pin, GPIOx, GPIO_PIN_RESET, htim, WAIT_PROCESS + WAIT_OVERHEAD) == -1) {
//        return -1;
//    }
//
//    // Wait for the line to go high again — end of this bit's low phase
//    if (wait_for_level_or_timeout(gpio_pin, GPIOx, GPIO_PIN_SET, htim, WAIT_READ + 2 * READ_OVERHEAD) == -1) {
//        return -1;
//    }
//
//    // Measure the high phase duration to decide 0 vs 1
//    int status = wait_for_level_or_timeout(gpio_pin, GPIOx, GPIO_PIN_RESET, htim, WAIT_READ * 2);
//    if (status == -1) {
//        return -1;
//    }
//
//    *bit_val = (status > WAIT_READ) ? 1 : 0;
//    return 0;
//}
//
//void start_signal(uint16_t gpio_pin, GPIO_TypeDef  *GPIOx, TIM_HandleTypeDef *htim){
//
//
//	// sets high, configures as output
//	switch_to_output(gpio_pin, GPIOx);
//	// pull low
//	HAL_GPIO_WritePin(GPIOx, gpio_pin, GPIO_PIN_RESET);
//	// hold low ~18ms (start signal)
//	HAL_Delay(START_LOW);
//	// release high
//	HAL_GPIO_WritePin(GPIOx, gpio_pin, GPIO_PIN_SET);
//	delay_us(START_HIGH, htim);
//	switch_to_input(gpio_pin, GPIOx);
//
//}
//
//int DHT_CheckResponse(uint16_t gpio_pin, GPIO_TypeDef  *GPIOx, TIM_HandleTypeDef *htim){
//
//
//	int status;
//	if((status = wait_for_level_or_timeout(gpio_pin, GPIOx, GPIO_PIN_RESET, htim,WAIT_PROCESS + WAIT_OVERHEAD)) == -1){
//		return -1;
//	}
//
//
//	status = wait_for_level_or_timeout(gpio_pin, GPIOx, GPIO_PIN_SET, htim,WAIT_PROCESS + WAIT_OVERHEAD);
//	return status;
//
//}
//
//DHT_Status DHT_ReadData(uint16_t gpio_pin, GPIO_TypeDef *GPIOx,
//		TIM_HandleTypeDef *htim, DHT_Data *result){
//	int bit_val;
//	int status;
//	uint8_t data[5] = {0};
//
//	uint8_t bit_position = 0x80;
//
//	start_signal(gpio_pin, GPIOx, htim);
//
//	status = DHT_CheckResponse(gpio_pin, GPIOx, htim);
//	if(status == -1){
//		return DHT_ERROR_NO_RESPONSE;
//	}
//
//
//	int i = 0;
//	while(i < RESPONSE_LENGTH){
//		int byte_index = i / 8;
//
//		status = read_bit(gpio_pin, GPIOx, htim, &bit_val);
//		if(status == -1) {
//			return DHT_ERROR_TIMEOUT_DATA;
//		}
//
//
//		if(bit_val){
//			data[byte_index] |= bit_position;
//		}
//
//		bit_position >>= 1;
//
//		if((i + 1) % 8 == 0){
//			bit_position = 0x80;
//		}
//
//		i++;
//
//	}
//
//	uint8_t checksum_calc = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
//
//	if (checksum_calc != data[4]) {
//	    return DHT_ERROR_CHECKSUM;
//	}
//	result->humidity = data[0];
//	result->humidity_decimal = data[1];
//	result->temperature = data[2];
//	result->temperature_decimal = data[3];
//	result->checksum = data[4];
//
//	return DHT_OK;
//
//}
