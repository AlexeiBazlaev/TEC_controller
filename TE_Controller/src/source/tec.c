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

__attribute__((optimize("O0")))
void TEC_L_set(bool	value){
	// switch off
	pin_set_output(PIN_LPGATE, true, 1);	
	pin_set_output(PIN_LNGATE, true, 0);
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	if(value){
		// L to +12
		pin_set_output(PIN_LPGATE, true, 0);
		}else{
		// L to GND
		pin_set_output(PIN_LNGATE, true, 1);
	}
}

void pin_set_output(uint8_t	pin, bool	output_flag, uint8_t	value){
	
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

static inline void pin_set_output_(uint8_t pin, bool output_flag, uint8_t value, uint8_t pull)
{	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	if(output_flag){
		port_pin_set_output_level(pin, value);
		config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
		}else{
		config_port_pin.direction = PORT_PIN_DIR_INPUT;
	}
	config_port_pin.input_pull = pull;
	port_pin_set_config(pin, &config_port_pin);
}


#if !defined DEBUG_TEC_SWITCHING
static inline void ReduceCurrent(float valueOld, uint8_t chanOld)
{	
	if(!chanOld) return;
	uint8_t i = 10;
	float delta = valueOld/i;
	float compare = valueOld;
	while(i--)	
	{		
		compare -= delta;
		tcc_set_compare_value(&tcc_instance, chanOld, PWM_GCLK_PERIOD*compare);
		vTaskDelay(10);
	}	
}

inline void TEC_Init(void)
{	
	tcc_reset(&tcc_instance);
	pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_UP);		
	pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_UP);		
	pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_DOWN);	
	pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_DOWN);		
}
__attribute__((optimize("O0")))
void	TEC_set_level(float	value)
{	
	static float valueOld = 0;
	static uint8_t chanOld = 0;
	
	if(fpclassify(value) == FP_ZERO || fpclassify(value) == FP_NAN)
	{
		ReduceCurrent(valueOld, chanOld);		
		TEC_Init();
		valueOld = 0;
		chanOld = 0;			
	}
	else
	{			
		if(value > 0.0)
		{			
			if(chanOld != PWM_CHANNEL_SNGATE)
			{
				ReduceCurrent(valueOld, chanOld);				
				chanOld = PWM_CHANNEL_SNGATE;				
				tcc_disable(&tcc_instance);//tcc_reset(&tcc_instance);
				pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_UP);
				pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_DOWN);									
								
				pin_set_output_(PIN_LPGATE, true, 0, PORT_PIN_PULL_DOWN);// to +12				
				pin_set_output_(PIN_SNGATE, false, 0, PORT_PIN_PULL_NONE);
				
				struct tcc_config config_tcc;
				tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
				config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;
				config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
				config_tcc.counter.period			= PWM_GCLK_PERIOD;
						
				config_tcc.pins.wave_out_pin[PWM_PIN_OUTPUT_SNGATE] = PIN_PA17F_TCC0_WO7;
				config_tcc.pins.wave_out_pin_mux[PWM_PIN_OUTPUT_SNGATE] = MUX_PA17F_TCC0_WO7;
				config_tcc.compare.match[PWM_CHANNEL_SNGATE] = PWM_GCLK_PERIOD*value;
								
				config_tcc.pins.enable_wave_out_pin[PWM_PIN_OUTPUT_SNGATE] = true;
				tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
				tcc_enable(&tcc_instance);
			}
			else
			{				
				tcc_set_compare_value(&tcc_instance, PWM_CHANNEL_SNGATE, PWM_GCLK_PERIOD*value);
			}				
		}
		else
		{
			if(chanOld != PWM_CHANNEL_SPGATE )
			{	
				ReduceCurrent(valueOld, chanOld);			
				chanOld = PWM_CHANNEL_SPGATE;
								
				tcc_disable(&tcc_instance);//tcc_reset(&tcc_instance);
				pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_UP);								
				pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_DOWN);				
							
				pin_set_output_(PIN_LNGATE, true, 1, PORT_PIN_PULL_UP); // to GND
				pin_set_output_(PIN_SPGATE, false, 1, PORT_PIN_PULL_NONE);
				
				struct tcc_config config_tcc;
				tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);				
				config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;
				config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
				config_tcc.counter.period			= PWM_GCLK_PERIOD;
											
				config_tcc.pins.wave_out_pin[PWM_PIN_OUTPUT_SPGATE] = PIN_PA16F_TCC0_WO6;
				config_tcc.pins.wave_out_pin_mux[PWM_PIN_OUTPUT_SPGATE] = MUX_PA16F_TCC0_WO6;
				config_tcc.compare.match[PWM_CHANNEL_SPGATE] = PWM_GCLK_PERIOD*value*(-1);				
				config_tcc.wave_ext.invert[PWM_PIN_OUTPUT_SPGATE]=true;
				
				config_tcc.pins.enable_wave_out_pin[PWM_PIN_OUTPUT_SPGATE] = true;
				tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
				tcc_enable(&tcc_instance);
			}
			else 
			{				
				tcc_set_compare_value(&tcc_instance, PWM_CHANNEL_SPGATE, PWM_GCLK_PERIOD*value*(-1));
			}
						
		}		
		valueOld = value;
	}
}
#elif defined DEBUG_TEC_SWITCHING_
__attribute__((optimize("O0")))
void	TEC_set_level(float	value)
{
	static float valueOld = 0;
	static uint8_t chanOld = 0;
	if(fpclassify(value) == FP_ZERO || fpclassify(value) == FP_NAN)
	{
		//tcc_reset(&tcc_instance);
		TEC_Init();
		valueOld = 0;
		chanOld = 0;
	}
	else
	{
		struct tcc_config config_tcc;
		if (fpclassify(valueOld) == FP_ZERO)
		{
			tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
			config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;
			config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
			config_tcc.counter.period			= PWM_GCLK_PERIOD;
		}
		
		uint8_t	pin_output, chan;
		uint32_t compare;
		if(value > 0.0)//if(fpclassify(value) > FP_ZERO)
		{
			chan=3;
			if(chanOld != 3)//if(fpclassify(valueOld) <= FP_ZERO)
			{
				ReduceCurrent(valueOld, chanOld, -1);
				//tcc_reset(&tcc_instance);
				TEC_Init();
				//port_pin_set_output_level(PIN_LNGATE, 0);
				//port_pin_set_output_level(PIN_SPGATE, 1);
				//pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_NONE);// PORT_PIN_PULL_UP);
				//pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_NONE);// PORT_PIN_PULL_DOWN);
				
				//port_pin_set_output_level(PIN_LPGATE, 0);
				//port_pin_set_output_level(PIN_SNGATE, 1);
				pin_set_output_(PIN_LPGATE, true, 0, PORT_PIN_PULL_DOWN);// to +12
				pin_set_output_(PIN_SNGATE, false, 0, PORT_PIN_PULL_NONE);//pin_set_output(PIN_SNGATE, false, 1);
				
				tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
				config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;
				config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
				config_tcc.counter.period			= PWM_GCLK_PERIOD;
				
				pin_output=7;
				config_tcc.pins.wave_out_pin[pin_output] = PIN_PA17F_TCC0_WO7;
				config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA17F_TCC0_WO7;
				config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value;
				
				config_tcc.pins.enable_wave_out_pin[pin_output] = true;
				tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
				tcc_enable(&tcc_instance);
			}
			else
			{
				compare = PWM_GCLK_PERIOD*value;
				tcc_set_compare_value(&tcc_instance, chan, compare);
			}
		}
		else// if(value < 0.0)//(fpclassify(value) < FP_ZERO)
		{
			chan=2;
			if(chanOld !=2 )//if(fpclassify(valueOld) >= FP_ZERO)
			{
				ReduceCurrent(valueOld, chanOld, 1);
				//tcc_reset(&tcc_instance);
				TEC_Init();
				//port_pin_set_output_level(PIN_LPGATE, 1);
				//port_pin_set_output_level(PIN_SNGATE, 0);
				//pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_NONE);// PORT_PIN_PULL_DOWN);
				//pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_NONE);// PORT_PIN_PULL_UP);
				
				//port_pin_set_output_level(PIN_LNGATE, 1);
				//port_pin_set_output_level(PIN_SPGATE, 0);
				pin_set_output_(PIN_LNGATE, true, 1, PORT_PIN_PULL_UP); // to GND
				pin_set_output_(PIN_SPGATE, false, 1, PORT_PIN_PULL_NONE);//pin_set_output(PIN_SPGATE, false, 0);
				
				tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
				config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;
				config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
				config_tcc.counter.period			= PWM_GCLK_PERIOD;
				
				pin_output=6;
				config_tcc.pins.wave_out_pin[pin_output] = PIN_PA16F_TCC0_WO6;
				config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA16F_TCC0_WO6;
				config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value*(-1);
				config_tcc.wave_ext.invert[pin_output]=true;
				
				config_tcc.pins.enable_wave_out_pin[pin_output] = true;
				tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
				tcc_enable(&tcc_instance);
			}
			else
			{
				compare = PWM_GCLK_PERIOD*value*(-1);
				tcc_set_compare_value(&tcc_instance, chan, compare);
			}
			
		}
		chanOld = chan;
		valueOld = value;
	}
}
#elif defined DEBUG_TEC_SWITCHING
inline void TEC_Init(void)
{
	tcc_reset(&tcc_instance);
	pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_UP);
	pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_DOWN);
	pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_UP);
	pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_DOWN);
}
__attribute__((optimize("O0")))
// Range from -1 to 1
void	TEC_set_level(float	value)
{
		
	/*pin_set_output(PIN_SPGATE, true, 1);
	pin_set_output(PIN_SNGATE, true, 0);
	pin_set_output(PIN_LPGATE, true, 1);
	pin_set_output(PIN_LNGATE, true, 0);*/
	/*port_pin_set_output_level(PIN_LPGATE, 1);
	port_pin_set_output_level(PIN_LNGATE, 0);	
	port_pin_set_output_level(PIN_SPGATE, 1);	
	port_pin_set_output_level(PIN_SNGATE, 0);*/
	//delay for 120ns = 20ns*6
	/*TEC_Init();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();*/
	
	if(fpclassify(value) == FP_ZERO || fpclassify(value) == FP_NAN)
	{
		TEC_Init();
		/*pin_set_output(PIN_SPGATE, true, 1);
		pin_set_output(PIN_SNGATE, true, 0);
		pin_set_output(PIN_LPGATE, true, 1);
		pin_set_output(PIN_LNGATE, true, 0);*/		
	}
	else
	{				
		struct tcc_config config_tcc;
		tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
		config_tcc.counter.clock_prescaler	= TCC_CLOCK_PRESCALER_DIV1;				
		config_tcc.compare.wave_generation	= TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
		config_tcc.counter.period			= PWM_GCLK_PERIOD;
				
		tcc_reset(&tcc_instance);
		
		uint8_t	pin_output, chan;
		if(value > 0.0)
		{
			//port_pin_set_output_level(PIN_LNGATE, 0);
			//port_pin_set_output_level(PIN_SPGATE, 1);
			pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_UP);						
			pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_DOWN);
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();						
			port_pin_set_output_level(PIN_LPGATE, 0);//pin_set_output(PIN_LPGATE, true, 0);// to +12			
			port_pin_set_output_level(PIN_SNGATE, 1);//pin_set_output(PIN_SNGATE, true, 1);//pin_set_output(PIN_SNGATE, false, 1);
			pin_output=7;
			chan=3;
			config_tcc.pins.wave_out_pin[pin_output] = PIN_PA17F_TCC0_WO7;
			config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA17F_TCC0_WO7;
			config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value;
		}
		else
		{	
			//port_pin_set_output_level(PIN_LPGATE, 1);			
			//port_pin_set_output_level(PIN_SNGATE, 0);			
			pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_DOWN);
			pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_UP);
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();							
			port_pin_set_output_level(PIN_LNGATE, 1);//pin_set_output(PIN_LNGATE, true, 1); // to GND
			port_pin_set_output_level(PIN_SPGATE, 0);//pin_set_output(PIN_SPGATE, true, 0);//pin_set_output(PIN_SPGATE, false, 0);			
			pin_output=6;
			chan=2;
			config_tcc.pins.wave_out_pin[pin_output] = PIN_PA16F_TCC0_WO6;
			config_tcc.pins.wave_out_pin_mux[pin_output] = MUX_PA16F_TCC0_WO6;
			config_tcc.compare.match[chan] = PWM_GCLK_PERIOD*value*(-1);
			config_tcc.wave_ext.invert[pin_output]=true;
		}
		config_tcc.pins.enable_wave_out_pin[pin_output] = true;
		tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
		tcc_enable(&tcc_instance);
	}
}

#else
inline void TEC_Init(void)
{	
	tcc_reset(&tcc_instance);
	pin_set_output_(PIN_LPGATE, true, 1, PORT_PIN_PULL_UP);		
	pin_set_output_(PIN_SPGATE, true, 1, PORT_PIN_PULL_UP);		
	pin_set_output_(PIN_LNGATE, true, 0, PORT_PIN_PULL_DOWN);	
	pin_set_output_(PIN_SNGATE, true, 0, PORT_PIN_PULL_DOWN);		
}
__attribute__((optimize("O0")))
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
#endif
#ifdef USE_ADN8831
extern struct dac_module dac_instance;
void	TEC_set_TEMPSET_volt(float	value)
{	
	float		Uref=3.3;
	uint16_t	dig_value = (value*1024)/Uref;
	dac_chan_write(&dac_instance, DAC_CHANNEL_0, dig_value);
}
#endif //USE_ADN8831