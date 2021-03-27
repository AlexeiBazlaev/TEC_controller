/*
 * dht.c
 *
 * Created: 04.01.2021 21:06:09
 *  Author: Lexus
 */ 

#include "dht.h"
#include "main.h"
#include <math.h>
#define lineDown()		port_pin_set_output_level(DHT_Pin, 0)
#define lineUp()		port_pin_set_output_level(DHT_Pin, 1)
#define getLine()		port_pin_get_input_level(DHT_Pin)
#define Delay(d)		vTaskDelay(d)

#define CPU_IRQ_enable()	cpu_irq_enable()
#define CPU_IRQ_disable()	cpu_irq_disable()
extern struct measured_params	m_params;
extern Controller_t Controller;
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
__attribute__((optimize("O0")))
uint8_t _DHT_getData(DHT_type t, DHT_data*	p_data) {
	uint8_t ret = 1;
	p_data->hum=0.0f;
	p_data->temp=0.0f;
	
	/* Запрос данных у датчика */	
	goToOutput();//Перевод пина "на выход"	
	lineDown();//Опускание линии данных на 1 мс
	Delay(2);//portYIELD();//
	portENTER_CRITICAL();	
	goToInput();//Подъём линии, перевод порта "на вход"
			
	/* Ожидание ответа от датчика */
	uint32_t timeout = 0;	
	while(getLine())//Ожидание спада
	{		
		if (timeout++ > DHT_timeout)
		{
			ret = 1;
			goto exit;
		}
	}
	timeout = 0;	
	while(!getLine())//Ожидание подъёма
	{					
		if (timeout++ > DHT_timeout)
		{
			ret = 2;
			goto exit;
		}
	}
	timeout = 0;	
	while(getLine())//Ожидание спада
	{		
		if (timeout++ > DHT_timeout)
		{
			ret = 3;
			goto exit;
		}
	}
	
	/* Чтение ответа от датчика */
	uint8_t rawData[5] = {0,0,0,0,0};
	uint32_t hT = 0, lT = 0;
	for(uint8_t a = 0; a < 5; a++) {
		for(uint8_t b = 7; b != 255; b--) {			
			//Пока линия в низком уровне, инкремент переменной lT
			while(!getLine())
			{
				if(lT++ > 60)
				{
					ret = 4;
					goto exit;
				}
			}
			//Пока линия в высоком уровне, инкремент переменной hT
			while(getLine())
			{
				if(hT++ > 60)
				{
					ret = 5;
					goto exit;
				}
			}			
			//Если hT больше lT, то пришла единица
			if(hT > lT) rawData[a] |= (1<<b);
			hT = 0; lT = 0;			
		}
	}	
	portEXIT_CRITICAL();	
	/* Проверка целостности данных */
	if((uint8_t)(rawData[0] + rawData[1] + rawData[2] + rawData[3]) == rawData[4])
	{
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
	}
	else
	{		
		return 6;
	}	
	return 0;
exit:	
	portEXIT_CRITICAL();	
	return ret;	
}

static float DewPoint(float temp, float hum)
{
	#define alfa 17.27f
	#define beta 237.7f
	float rslt = 0;
	float gamma;
	if (hum <= 0)
	hum = 0.0025;
	gamma = alfa*temp/(beta+temp) + logf((float)hum/100);
	rslt = beta*gamma/(alfa-gamma);
	return rslt;
}

void	measurement_DHT22(){
	DHT_data d;
	uint8_t	ret = DHT_getData(DHT22, &d);		
	Controller.temps.DHT22_Temp = ret? (0.0/0.0) : (d.temp);//m_params.DHT22_Temp = 0.0/0.0;
	Controller.temps.DHT22_Hum	= ret? (0.0/0.0) : (d.hum);//m_params.DHT22_Hum = 0.0/0.0;//m_params.DHT22_comError++;				
	if(fpclassify(Controller.temps.DHT22_Temp) != FP_NAN && fpclassify(Controller.temps.DHT22_Hum) != FP_NAN)
		Controller.temps.dewPoint = DewPoint(Controller.temps.DHT22_Temp, Controller.temps.DHT22_Hum);
}
