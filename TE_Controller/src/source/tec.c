/*
 * tec.c
 *
 * Created: 04.01.2021 17:08:34
 *  Author: Lexus
 */ 

#include "tec.h"
#include <arm_math.h>
#include "adc_user.h"
#include "dac_user.h"
struct tcc_module tcc_instance;
extern struct dac_module dac_instance;
bool VTEC_read(float	*p_Vin, float	*p_VTEC){
	float Vin=  adc_get_V_spec(chan_VTEC);
	if(p_Vin){
		*p_Vin = Vin;
	}
	if(p_VTEC){
		//float	K __attribute__((used)) =3.67 ;
		//		*p_VTEC = ((Vin - 1.25)*K)/0.25;
		*p_VTEC = ((Vin - 1.25))/0.25;
	}
	return true;
}

bool ITEC_read(float	*p_Vin, float	*p_ITEC){
	float Vin=  adc_get_V_spec(chan_ITEC);
	if(p_Vin){
		*p_Vin = Vin;
	}
	if(p_ITEC){
		float	Rsense = 0.005;
		*p_ITEC = (Vin-1.25)/(25*Rsense);
	}
	return true;
}

void TEC_L_set(bool	value){

	// switch off
	pin_set_output(PIN_LPGATE, true, 1);
	pin_set_output(PIN_LNGATE, true, 0);
	
	if(value){
		// L to +12
		pin_set_output(PIN_LPGATE, true, 0);
		}else{
		// L to GND
		pin_set_output(PIN_LNGATE, true, 1);
	}
}

void	pin_set_output(uint8_t	pin, bool	output_flag, uint8_t	value){
	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	if(output_flag){
		port_pin_set_output_level(pin, value);
		config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
		}else{
		config_port_pin.direction = PORT_PIN_DIR_INPUT;
	}
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(pin, &config_port_pin);
}

// Range from -1 to 1
void	TEC_set_level(float	value)
{
	tcc_reset(&tcc_instance);
	if(fpclassify(value) == FP_ZERO || fpclassify(value) == FP_NAN)
	{		
		pin_set_output(PIN_SPGATE, true, 1);
		pin_set_output(PIN_SNGATE, true, 0);
		pin_set_output(PIN_LPGATE, true, 1);
		pin_set_output(PIN_LNGATE, true, 0);
	}
	else
	{
		struct port_config config_port_pin;
		port_get_config_defaults(&config_port_pin);
		config_port_pin.direction	= PORT_PIN_DIR_INPUT;
		config_port_pin.input_pull	= PORT_PIN_PULL_NONE;
		port_pin_set_config(PIN_SNGATE, &config_port_pin);
		
		struct tcc_config config_tcc;
		tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
		config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;				
		config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
		config_tcc.counter.period			= PWM_GCLK_PERIOD;
		
		uint8_t	pin_output, chan;
		if(value > 0.0)
		{
			TEC_L_set(true);	// to +12
			pin_set_output(PIN_SPGATE, true, 1);
			pin_set_output(PIN_SNGATE, false, 0);
			pin_output=7;
			chan=3;
			config_tcc.pins.wave_out_pin[pin_output] = PIN_PA17F_TCC0_WO7;
			config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA17F_TCC0_WO7;

			config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value;
		}
		else
		{
			TEC_L_set(false);	// to GND
			pin_set_output(PIN_SPGATE, false, 0);
			pin_set_output(PIN_SNGATE, true, 0);
			pin_output=6;
			chan=2;
			config_tcc.pins.wave_out_pin[pin_output] = PIN_PA16F_TCC0_WO6;
			config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA16F_TCC0_WO6;
			config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value*(-1.0);
			config_tcc.wave_ext.invert[pin_output]=true;
		}
		config_tcc.pins.enable_wave_out_pin[pin_output] = true;
		tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
		tcc_enable(&tcc_instance);
	}
}

void	TEC_set_TEMPSET_volt(float	value)
{	
	float		Uref=3.3;
	uint16_t	dig_value = (value*1024)/Uref;
	dac_chan_write(&dac_instance, DAC_CHANNEL_0, dig_value);
}