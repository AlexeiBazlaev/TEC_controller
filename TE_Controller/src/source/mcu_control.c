/*
 * mcu_control.c
 *
 * Created: 12.01.2021 23:17:16
 *  Author: Lexus
 */ 
#include "mcu_control.h"
#include <asf.h>
#include "ntc.h"
#include "dht.h"
#include "adn8831.h"
#include "tm_onewire.h"
#include "tm_ds18b20.h"
#include "adc_user.h"
void	MCU_control_init(void){
	
}

float	GetTecCurrent(float v_cs, float v_ref, float r_cs, float gain)
{	
	return ((v_cs - v_ref)/(r_cs*gain));
}

void	MCU_control(void){	
	char	t_TEC_str[MAX_STR_FLOAT];
	char	t_MCU_str[MAX_STR_FLOAT];
	TM_OneWire_t	ow_instance;
	// MCU temp
	float	t = NTC_MCU_get_temp(NULL);
	make_float2str(t_MCU_str, MAX_STR_FLOAT, (float)t);
	
	// TEC temp
	float		t_TEC = NTC_TEC_get_temp(NULL, NULL);
	make_float2str(t_TEC_str, MAX_STR_FLOAT, (float)t_TEC);
	
	// voltage LFB, SFB, CS
	float	Vin_LFB, Vin_SFB, Vin_CS;//, Vin_VTEC;
	Vin_LFB = adc_get_V_spec(chan_LFB);
	Vin_SFB = adc_get_V_spec(chan_SFB);
	Vin_CS = adc_get_V_spec(chan_CS);

	char	Vin_LFB_str[MAX_STR_FLOAT];
	char	Vin_SFB_str[MAX_STR_FLOAT];
	char	Vin_CS_str[MAX_STR_FLOAT];
	
	make_float2str(Vin_LFB_str, MAX_STR_FLOAT, (float)Vin_LFB);
	make_float2str(Vin_SFB_str, MAX_STR_FLOAT, (float)Vin_SFB);
	make_float2str(Vin_CS_str, MAX_STR_FLOAT, (float)Vin_CS);
	
	DHT_data d;
	uint8_t	ret = DHT_getData(DHT22, &d);
	char	DHT22_Temp[MAX_STR_FLOAT];
	char	DHT22_Hum[MAX_STR_FLOAT];
	make_float2str(DHT22_Temp, MAX_STR_FLOAT, (float)d.temp);
	make_float2str(DHT22_Hum, MAX_STR_FLOAT, (float)d.hum);
	if(ret){
		printf("DHT22 error: %d\r\n",(int)ret);
	}
	
	// DS1820
	char	DS1820_Temp_str[MAX_STR_FLOAT];
	//bool 	ow_connected __attribute__((used)) =false;
	float	DS1820_temp=0.0;
	//cpu_irq_disable();
	if (TM_OneWire_First(&ow_instance)) {
		//ow_connected=true;
		if(TM_DS18B20_Is(ow_instance.ROM_NO)){
			while(!TM_DS18B20_AllDone(&ow_instance)){};
			ret = TM_DS18B20_Read(&ow_instance, ow_instance.ROM_NO, &DS1820_temp);
			if(!ret){
				printf("TM_DS18B20_Read() - error\r\n");
			}
			ret = TM_DS18B20_Start(&ow_instance, ow_instance.ROM_NO);
			if(!ret){
				printf("TM_DS18B20_Start() - ROM isn't correct\r\n");
			}
		}
	}
	//cpu_irq_enable();
	make_float2str(DS1820_Temp_str, MAX_STR_FLOAT, (float)DS1820_temp);
	
	//	printf("Info: t_MCU: %10s, t_TEC:%10s, Vin_LFB: %6s, Vin_SFB: %6s, Vin_CS: %6s, DHT22_Temp: %s, DHT22_Hum: %s, OW_STATE: %d, DS1820_Temp: %s\r\n",t_MCU_str, t_TEC_str, Vin_LFB_str, Vin_SFB_str, Vin_CS_str, DHT22_Temp, DHT22_Hum, (int)ow_connected, DS1820_Temp_str);
	//	printf("Info: t_MCU: %10s, t_TEC:%10s, RS: %d, DHT22_Temp: %s, DHT22_Hum: %s, OW_STATE: %d, DS1820_Temp: %s\r\n",t_MCU_str, t_TEC_str, (int)rs_power, DHT22_Temp, DHT22_Hum, (int)ow_connected, DS1820_Temp_str);
	vTaskDelay(2000);//delay_ms(2000);
}