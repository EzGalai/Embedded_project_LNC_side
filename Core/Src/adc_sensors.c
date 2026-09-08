/*
 * adc_sensors.c — see adc_sensors.h.
 */

#include "adc_sensors.h"
#include "main.h" /* for hadc1 */

static uint16_t ADC_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0;
    }

    uint16_t value = 0;
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            value = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }

    return value;
}

uint16_t ADC_ReadBatteryVoltage(void)
{
    uint16_t raw = ADC_ReadChannel(ADC_CHANNEL_5);
    return (uint16_t)(((uint32_t)raw * 3300) / 4095);
}

uint16_t ADC_ReadLight(void)
{
    return ADC_ReadChannel(ADC_CHANNEL_6);
}
