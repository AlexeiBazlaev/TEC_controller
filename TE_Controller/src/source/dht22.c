/*
 * dht22.c
 *
 * Created: 04.01.2021 21:06:09
 *  Author: Lexus
 */ 

#include "dht22.h"
#include <asf.h>

#define lineDown()		port_pin_set_output_level(DHT_Pin, 0)
#define lineUp()		port_pin_set_output_level(DHT_Pin, 1)
#define getLine()		port_pin_get_input_level(DHT_Pin)
#define Delay(d)		vTaskDelay(d)

#define CPU_IRQ_enable()	cpu_irq_enable()
#define CPU_IRQ_disable()	cpu_irq_disable()
extern struct measured_params	m_params;
uint32_t hT = 0, lT = 0;
bool startGetData = false, stopGetData = false;
volatile uint32_t startTick = 0, stopTick =0;
volatile uint16_t  period = 0;
#define BIT0 4080//48Mhz*(55us+30us)
#define BIT1 5568//48Mhz*(48us+68us)

uint8_t rawData[5] = {0,0,0,0,0};
static void goToOutput(void) {

	// по умолчанию на линии высокий уровень
	port_pin_set_output_level(DHT_Pin, 1);
	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(DHT_Pin, &config_port_pin);
}

static void goToInput(void) {
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(DHT_Pin, &config_port_pin);
}

uint8_t DHT_getData(DHT_type t, DHT_data*	p_data){
	uint8_t	ret;	
	ret = _DHT_getData(t, p_data);	
	return ret;
}


void configure_extint_callbacks(void);
void extint_detection_callback(void);



void configure_extint_callbacks(void)
{	
	extint_register_callback(extint_detection_callback,
							 DHT_Pin,
							 EXTINT_CALLBACK_TYPE_DETECT);	
	extint_chan_enable_callback(DHT_Pin,
								EXTINT_CALLBACK_TYPE_DETECT);	
}

inline static uint32_t GetSysTick(void)
{
	return SysTick->VAL;
}
inline static uint32_t GetReloadTick(void)
{
	return SysTick->LOAD;
}

uint8_t a = 0;
uint8_t b = 7;
void extint_detection_callback(void)
{
	if(!startGetData)
	{
		startGetData = true;
		a = 0;
		b = 7;		
	/*	return;
	}
	if(!startTick)
	{*/
		startTick = GetSysTick();		
		if(!startTick) startTick = GetReloadTick();
		return;
	}
	else
	{
		stopTick = GetSysTick();		
		if (stopTick < startTick)
			period = startTick - stopTick;
		else
			period = startTick + (GetReloadTick() - stopTick);
		startTick = 0;
		stopTick = 0;
		if (a < 5)
		{
			if (b != 255)
			{
				if(period>BIT0)
				rawData[a] |= (1<<b);
				b--;				
				return;
			}
			a++;										
			b = 7;
		}
		else
		{			
			startGetData = false;	
			a = 0;
			b = 7;
		}
	}	
}

void DHT22_EIC_configure(void)
{	
	struct extint_chan_conf DHT22_EIC_config;
	extint_chan_get_config_defaults(&DHT22_EIC_config);
	DHT22_EIC_config.gpio_pin            = DHT_Pin,
	DHT22_EIC_config.gpio_pin_mux        = MUX_PA15A_EIC_EXTINT15,
	DHT22_EIC_config.gpio_pin_pull       = EXTINT_PULL_UP,
	DHT22_EIC_config.wake_if_sleeping    = true,
	DHT22_EIC_config.filter_input_signal = false,
	DHT22_EIC_config.detection_criteria  = EXTINT_DETECT_FALLING;
	extint_chan_set_config( DHT_Pin, &DHT22_EIC_config);
}


uint8_t _DHT_getData(DHT_type t, DHT_data*	p_data)
{
	p_data->hum=0.0f;
	p_data->temp=0.0f;
#ifdef DEBUG_EXT_INT_CONTROLLER	
	configure_extint_callbacks();
	while(1){};
#endif		
	/* Запрос данных у датчика */
	//Перевод пина "на выход"
	//extint_chan_disable_callback(DHT_Pin,EXTINT_CALLBACK_TYPE_DETECT);
	goToOutput();
	//Опускание линии данных на 15 мс
	lineDown();			
	configure_extint_callbacks();		
	Delay(1);
	DHT22_EIC_configure();
	//Подъём линии, перевод порта "на вход"
	//lineUp();	
	//goToInput();
			
	/* Ожидание ответа от датчика */
	/*uint32_t timeout = 0;
	//Ожидание спада
	while(getLine()) {
		timeout++;
		if (timeout > DHT_timeout)
			return 0x01;
	}
	timeout = 0;
	
	//Ожидание подъёма
	while(!getLine()) {
		timeout++;
		if (timeout > DHT_timeout)
			return 0x02;
	}
	timeout = 0;
	
	
	//Ожидание спада
	while(getLine()) {
		timeout++;
		if (timeout > DHT_timeout)
			return 0x03;
	}*/
	
	/* Чтение ответа от датчика */
	//rawData[5] = {0,0,0,0,0};
	while(!startGetData) 
		portYIELD();
	while(startGetData){} 
		//portYIELD();
	extint_chan_disable_callback(DHT_Pin,EXTINT_CALLBACK_TYPE_DETECT);
	/* Проверка целостности данных */
	if((uint8_t)(rawData[0] + rawData[1] + rawData[2] + rawData[3]) == rawData[4]) {
		//Если контрольная сумма совпадает, то конвертация и возврат полученных значений
		if (t == DHT22) {
			p_data->hum = (float)(((uint16_t)rawData[0]<<8) | rawData[1])*0.1f;
			//Проверка на отрицательность температуры
			if(!(rawData[2] & (1<<7))) {
				p_data->temp = (float)(((uint16_t)rawData[2]<<8) | rawData[3])*0.1f;
				}	else {
				rawData[2] &= ~(1<<7);
				p_data->temp = (float)(((uint16_t)rawData[2]<<8) | rawData[3])*-0.1f;
			}
		}
		if (t == DHT11) {
			p_data->hum = (float)rawData[0];
			p_data->temp = (float)rawData[2];
		}
		}else{
		return 0x04;
	}
	
	return 0;
}

void	measurement_DHT22(){
	DHT_data d;
	uint8_t	ret = DHT_getData(DHT22, &d);
	if(ret){
		printf("DHT22 error: %d\r\n",(int)ret);
		}else{
		m_params.DHT22_Temp = d.temp;
		m_params.DHT22_Hum = d.hum;
	}
}
