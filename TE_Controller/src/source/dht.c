/*
 * dht.c
 *
 * Created: 04.01.2021 21:06:09
 *  Author: Lexus
 */ 

#include "dht.h"

#define lineDown()		port_pin_set_output_level(DHT_Pin, 0)
#define lineUp()		port_pin_set_output_level(DHT_Pin, 1)
#define getLine()		port_pin_get_input_level(DHT_Pin)
#define Delay(d)		delay_ms(d)

#define CPU_IRQ_enable()	cpu_irq_enable()
#define CPU_IRQ_disable()	cpu_irq_disable()
extern struct measured_params	m_params;
static void goToOutput(void) {

	// по умолчанию на линии высокий уровень
	port_pin_set_output_level(DHT_Pin, 1);
	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(DHT_Pin, &config_port_pin);
}

static void goToInput(void) {
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(DHT_Pin, &config_port_pin);
}

uint8_t DHT_getData(DHT_type t, DHT_data*	p_data){
	uint8_t	ret;
	CPU_IRQ_disable();
	ret = _DHT_getData(t, p_data);
	CPU_IRQ_enable();
	return ret;
}

uint8_t _DHT_getData(DHT_type t, DHT_data*	p_data) {
	p_data->hum=0.0f;
	p_data->temp=0.0f;
	
	
	/* Запрос данных у датчика */
	//Перевод пина "на выход"
	goToOutput();
	//Опускание линии данных на 15 мс
	lineDown();
	Delay(15);
	//Подъём линии, перевод порта "на вход"
	lineUp();
	goToInput();
	
	/* Ожидание ответа от датчика */
	uint32_t timeout = 0;
	//Ожидание спада
	while(getLine()) {
		timeout++;
		if (timeout > DHT_timeout) return 0x01;
	}
	timeout = 0;
	//Ожидание подъёма
	while(!getLine()) {
		timeout++;
		if (timeout > DHT_timeout) return 0x02;
	}
	timeout = 0;
	//Ожидание спада
	while(getLine()) {
		timeout++;
		if (timeout > DHT_timeout) return 0x03;
	}
	
	/* Чтение ответа от датчика */
	uint8_t rawData[5] = {0,0,0,0,0};
	for(uint8_t a = 0; a < 5; a++) {
		for(uint8_t b = 7; b != 255; b--) {
			uint32_t hT = 0, lT = 0;
			//Пока линия в низком уровне, инкремент переменной lT
			while(!getLine()) lT++;
			//Пока линия в высоком уровне, инкремент переменной hT
			while(getLine()) hT++;
			//Если hT больше lT, то пришла единица
			if(hT > lT) rawData[a] |= (1<<b);
			//			printf("DEBUG: %d, %d]r]n",(int)hT, (int)lT);
		}
	}
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
