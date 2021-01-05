/*
 * ntc.c
 *
 * Created: 04.01.2021 18:57:53
 *  Author: Lexus
 */ 
#include "ntc.h"
#include <arm_math.h>


#define MCU_CONTROL
// ADC value to R
float NTC_MCU_value2R(uint16_t	value)
{
	float	R10 = 17800.0;
	float	R11 = 7680.0;

	float	Q = adc_get_Q(value);
		
	return ((R10*Q)/(1 - Q) - R11);
}

float NTC_TEC_value2R(uint16_t	value)
{		
	#ifdef MCU_CONTROL
	float	R32 = 17800.0;
	float	R36 = 7680.0;
	float	Q = adc_get_Q(value);
		
	return ((R32*Q)/(1 - Q) - R36);
	#else
	float	R36 = 7680.0;
	float	Uref = 2.5/2.;
	float	Uvcc = 3.3;
	float	Q = adc_get_Q(value);
		
	return R36/(Uref/(Q*Uvcc) - 1);
	#endif
}

// Resistance to Temp
float NTC_R2T(float R)
{
	float	K0 = 273.15;
	float	T0 = 25.0 + K0;
	float	R0 = 10000.0;
		
	float	B  = 3863.737706;
	return (1.0/((log(R/R0)/B) + (1/T0)) - K0);
}

// Read ADC and calculate temperature in Cenlsius.
float NTC_MCU_get_temp(void)
{
	uint16_t adc_val = adc_read_value_spec(chan_NTC_MCU);
		
	float	R = NTC_MCU_value2R(adc_val);
	float	t = NTC_R2T(R);
	return 	t;
}

float NTC_TEC_get_temp(uint16_t	*p_value, float	*p_R)
{
	uint16_t adc_val = adc_read_value_spec(chan_NTC_TEC);
		
	if(p_value){
		*p_value = adc_val;
	}
		
	float	R = NTC_TEC_value2R(adc_val);
		
	if(p_R){
		*p_R = R;
	}
	float	t = NTC_R2T(R);
	return 	t;
}