/*
 * adc.c
 *
 * Created: 03.01.2021 23:33:58
 *  Author: Lexus
 */ 
#include "adc_user.h"
#include <asf.h>
struct adc_module adc_instance;
// init ADC
void configure_adc(void)
{
	struct adc_config config_adc;
	/*ADC_GAINCORR_Type gainCorr = {
		.bit.GAINCORR = 1
	};
	ADC_OFFSETCORR_Type offsetCorr = {
		.reg = -52
	};*/
	adc_get_config_defaults(&config_adc);

	config_adc.positive_input = ADC_POSITIVE_INPUT_PIN1;
	config_adc.negative_input = ADC_NEGATIVE_INPUT_GND;
	config_adc.reference = ADC_REFERENCE_INTVCC0;
		
	config_adc.resolution = ADC_RESOLUTION_12BIT;
	//config_adc.gain_factor = ADC_GAIN_FACTOR_1X;
	//config_adc.freerunning = true;
	config_adc.correction.correction_enable = true;
	config_adc.correction.offset_correction = -16;//ADC_OFFSETCORR_OFFSETCORR(offsetCorr.reg);//
	config_adc.correction.gain_correction = (1<<11);//1.0ADC_GAINCORR_GAINCORR(gainCorr.reg);//
	
	config_adc.gain_factor = ADC_GAIN_FACTOR_DIV2;
	config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV512; // prescaler influence to accuracy of measures. Don't set less then 32
		
	adc_init(&adc_instance, ADC, &config_adc);
		
	adc_enable(&adc_instance);
	//adc_start_conversion(&adc_instance);
}

/*uint16_t adc_read_value(void)
{	
	uint16_t result = 0xFFFF;
	adc_read(&adc_instance, &result);
	return result;
}*/
uint16_t adc_read_value(void){
	adc_start_conversion(&adc_instance);
	uint16_t result;
	do {		
	} while (adc_read(&adc_instance, &result) == STATUS_BUSY);
	return result;
}

uint16_t adc_read_value_spec(ADC_chan_t	chan)
{
	adc_set_positive_input(&adc_instance, chan);
	return adc_read_value();
}

float	adc_get_Q(uint16_t	value)
{	
	float	gane_factor=0.5;
	float	vref_k=1.0/1.48;
	float	ADC_range = 4096.0;
	return (value*vref_k)/(gane_factor*ADC_range);
}

float	adc_get_V(uint16_t	value)
{
	float	Uvcc=3.3;
	return adc_get_Q(value) * Uvcc;
}