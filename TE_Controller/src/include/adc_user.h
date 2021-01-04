/*
 * adc.h
 *
 * Created: 03.01.2021 23:34:13
 *  Author: Lexus
 */ 


#ifndef ADC_H_
#define ADC_H_

#define	chan_NTC_MCU ADC_POSITIVE_INPUT_PIN1
#define	chan_NTC_TEC ADC_POSITIVE_INPUT_PIN5
#define	chan_LFB	 ADC_POSITIVE_INPUT_PIN19
#define	chan_CS		 ADC_POSITIVE_INPUT_PIN4
#define	chan_SFB	 ADC_POSITIVE_INPUT_PIN18
#define	chan_VTEC	 ADC_POSITIVE_INPUT_PIN6
#define	chan_ITEC	 ADC_POSITIVE_INPUT_PIN7

typedef enum adc_positive_input ADC_chan_t;

void configure_adc(void);
uint16_t adc_read_value(void);
float	adc_get_V(uint16_t	value);
float	adc_get_Q(uint16_t	value);
uint16_t	adc_read_value_spec(ADC_chan_t);
float		adc_get_V_spec(ADC_chan_t	chan);
#endif /* ADC_H_ */