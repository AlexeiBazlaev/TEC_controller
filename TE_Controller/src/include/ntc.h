/*
 * ntc.h
 *
 * Created: 04.01.2021 18:57:42
 *  Author: Lexus
 */ 


#ifndef NTC_H_
#define NTC_H_
#include <asf.h>

	// ADC value to R
	float NTC_MCU_value2R(uint16_t	value);

	float NTC_TEC_value2R(uint16_t	value);

	// Resistance to Temp
	float NTC_R2T(float R);

	// Read ADC and calculate temperature in Cenlsius.
	float NTC_MCU_get_temp(void);

	float NTC_TEC_get_temp(uint16_t	*p_value, float	*p_R);


#endif /* NTC_H_ */