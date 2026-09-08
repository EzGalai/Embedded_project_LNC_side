/*
 * adc_sensors.h — ADC-based sensor readers: potentiometer (BATTERY_VOLTAGE
 * stand-in) and photoresistor (LIGHT). Both share ADC1, reconfigured per
 * read since they're only sampled occasionally (Monitor's 5s loop), not on
 * a continuous scan sequence. Returned in their true wire units (mV, raw
 * ADC) — any percentage-for-display conversion happens at the print/log
 * call site, not here.
 */

#ifndef INC_ADC_SENSORS_H_
#define INC_ADC_SENSORS_H_

#include <stdint.h>

/**
 * @brief Reads the potentiometer (standing in for BATTERY_VOLTAGE) and
 * converts it to millivolts, assuming a 3.3V ADC reference.
 * @return Voltage in mV.
 */
uint16_t ADC_ReadBatteryVoltage(void);

/**
 * @brief Reads the photoresistor (LIGHT). Returned unscaled — PROJECT_PLAN.md
 * §3.5 defines no unit for this field; the raw 12-bit value keeps calibration
 * in the LIGHT_MIN threshold, not baked into a conversion formula here.
 * @return Raw ADC value (0-4095).
 */
uint16_t ADC_ReadLight(void);

#endif /* INC_ADC_SENSORS_H_ */
