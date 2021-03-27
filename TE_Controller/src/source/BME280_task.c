/*
 * BME280_task.c
 *
 * Created: 23.02.2019 8:57:03
 *  Author: Lexus
 */
#include <asf.h>
//#include <atmel_start.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include "util/delay.h"

// Scheduler includes.
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "timers.h"
#include "semphr.h"
#include "queue.h"
#include "BME280_task.h"
//#include "twi_master.h"
#include "bme280.h"
#include "bme280_defs.h"
//#include "bmp280.h"
//#include "bmp280_defs.h"
//#include "hd44780.h"
#include "i2c_master_user.h"

//#include "lcd.h"
#define BME280_Task_STACK_SIZE ( configMINIMAL_STACK_SIZE *2 )
#define BME280_FLOAT_ENABLE

//extern xSemaphoreHandle xSemaphoreDisplay;
//extern xQueueHandle xQueueMenu;

//extern void I2C_0_writeNBytes(i2c_address_t address, void *data, size_t len);
//extern void I2C_0_readNBytes(i2c_address_t address, void *data, size_t len);
extern struct i2c_master_module i2c_master_instance;


uint8_t read_buffer[DATA_LENGTH];
sensorData_t sensor = {
	.Press = 200,
	.Hum = 0,
	.Hum1 = 0,
	.Temp = 100,
};
/**
*	\fn static int8_t Read(char dev_id, uint8_t reg_addr, uint8_t *temp_buff, uint16_t temp_len)
*	\brief Read from I2C
*/
static int8_t Read(char dev_id, uint8_t reg_addr, uint8_t *temp_buff, uint16_t temp_len)
{		
	//uint8_t data=reg_addr;
	//I2C_0_writeNBytes(dev_id, &data, 1);
	//I2C_0_readNBytes(dev_id, temp_buff, temp_len);	
	uint16_t timeout = 0;
	struct i2c_master_packet packet = {
		.address     = dev_id,
		.data_length = 1,
		.data        = &reg_addr,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};	
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) !=
	STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == TIMEOUT) {
			return BME280_E_COMM_FAIL;
		}
	}
	packet.data = read_buffer;
	while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) !=
	STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == TIMEOUT) {
			return BME280_E_COMM_FAIL;
		}
	}
	return BME280_OK;
}

/**
*	\fn static int8_t Write(char dev_id, uint8_t reg_addr, uint8_t *temp_buff, uint16_t temp_len)
*	\brief Write to I2C
*/
static int8_t Write(char dev_id, uint8_t reg_addr, uint8_t *temp_buff, uint16_t temp_len)
{		
	uint8_t data[20]={reg_addr, };
	strncat((char*)data, (const char*)temp_buff, temp_len);
	//I2C_0_writeNBytes(dev_id, data, temp_len+1);
	uint16_t timeout = 0;
	struct i2c_master_packet packet = {
		.address     = dev_id,
		.data_length = temp_len+1,
		.data        = data,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) !=
	STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == TIMEOUT) {
			return BME280_E_COMM_FAIL;
		}
	}	
	return BME280_OK;		
}

/**
*	\fn static void Delay(uint32_t period)
*	\brief Delay
*/
static void Delay(uint32_t period)
{	
	vTaskDelay(period);//_delay_ms(period);//
}

/**
*	\fn static void print_sensor_data(struct bme280_data *comp_data)
*	\brief Print sensor data
*/
__attribute__((used))
static void print_sensor_data(struct bme280_data *comp_data)
{	
#ifndef BME280_FLOAT_ENABLE
		printf("%.2f, %.2f, %.2f", (int)comp_data->pressure, (int)comp_data->humidity, (int)comp_data->temperature);
	
#else	
	printf(" %dkPa %d%% %d°C", (int)comp_data->pressure, (int)comp_data->humidity, (int)comp_data->temperature);
#endif
}

/**
*	\fn static void print_sensor_data(struct bme280_data *comp_data)
*	\brief Print dew_point data
*/
__attribute__((used))
static void print_dew_point(int32_t *dewPoint, struct bme280_data *comp_data)
{	
	printf(" %d°C  %d%% %d°C", *(int*)dewPoint, (int)comp_data->humidity, (int)comp_data->temperature);	
}



float DewPoint(float temp, float hum)
{
	#define alfa 17.27f
	#define beta 237.7f
	float rslt = 0;
	float gamma;
	if (hum <= 0)
		hum = 0.0025;
	gamma = alfa*temp/(beta+temp) + log((float)hum/100);
	rslt = beta*gamma/(alfa-gamma);
	return rslt;
}


struct bme280_dev bme280 = {
	.chip_id	= 0x00,
	.dev_id		= BME280_BUS_ADDR,
	.intf		= BME280_I2C_INTF,
	.read		= (bme280_com_fptr_t)Read,
	.write		= (bme280_com_fptr_t)Write,
	.delay_ms	= (bme280_delay_fptr_t)Delay,
	/* Recommended mode of operation: Indoor navigation */
	.settings.osr_h = BME280_OVERSAMPLING_8X,
	.settings.osr_p = BME280_OVERSAMPLING_8X,
	.settings.osr_t = BME280_OVERSAMPLING_8X,
	.settings.filter = BME280_FILTER_COEFF_8,
	.settings.standby_time = BME280_STANDBY_TIME_10_MS,
};

extern uint8_t error_BME280;
extern xTimerHandle xTimerBME280;
int8_t InitBME280(void)
{
	int8_t rslt /*__attribute__((used))*/ = BME280_E_COMM_FAIL;
	uint8_t settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;	
	//xTimerReset(xTimerBME280, 0);
	//xTimerStart(xTimerBME280, 0);
	//taskENTER_CRITICAL();
	rslt = bme280_init(&bme280);
	
	if(rslt!=BME280_OK)
		return rslt;
	rslt = bme280_set_sensor_settings(settings_sel, &bme280);
	if(rslt!=BME280_OK)
		return rslt;
	rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, &bme280);
	if(rslt!=BME280_OK)
		return rslt;	
		
	//xTimerStop(xTimerBME280, 0);
	error_BME280=0;
	return rslt;
}

static Timer_t bme280Timer;

inline void TimerStart(Timer_t *timer, TickType_t timeout_ms)
{
	if (timer->timeout) return;
	timer->timeout	= timeout_ms;
	timer->timeStart 	= xTaskGetTickCount();
	timer->timeStart = timer->timeStart ? timer->timeStart : 1;
}

inline void TimerStop(Timer_t *timer)
{
	timer->timeout	= 0;
}

bool IsTimeOut(Timer_t *timer)
{
	if( timer->timeout == 0 ){}
	else if ((xTaskGetTickCount() - timer->timeStart) >= timer->timeout )
	{
		TimerStop(timer);
		return TRUE;
	}
	return FALSE;
}

void BME280Timer_Handler(void)
{
	if( IsTimeOut(&bme280Timer) )
	{
#ifdef DEBUG
		__BKPT();
#endif		
		NVIC_SystemReset();
	}
}


struct bme280_data comp_data;
uint32_t comp_pres = 960;
int32_t comp_temp;
int8_t GetDataBME280(void)
{
	int8_t rslt /*__attribute__((used))*/ = BME280_E_COMM_FAIL;			
	rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &bme280);	
	if(rslt!=BME280_OK)
		return rslt;		
	return rslt;
}

//extern void vALARM_Task();
//extern void vMOTOR_Task();
//extern uint8_t HumidityApproximation(uint8_t *time, uint8_t *hum, uint8_t len, uint8_t timeExtra);

int32_t dewPoint;


//volatile uint8_t len=0;
//volatile float hum[DATA_LEN+1]={0,};
//volatile uint8_t time[DATA_LEN];

xTaskHandle xAlarmTaskHandle=NULL;
//portTASK_FUNCTION (vBME280_task, pvParameters)
//{
	//static int8_t rslt __attribute__((used)) = BME280_OK;	
	////uint8_t settings_sel;
	////int32_t dewPoint;//, oldDP=0;
	////double oldPress=0, oldRH=0;
	////uint8_t mess=1;
	//int8_t start = 2;
	////portTickType xToggleDisplayTick = 0;
	//
	////volatile uint16_t timeExtra=60000;//DATA_LEN*10;
		//
	////uint32_t delay=1000;
	//TimerStart(&bme280Timer, BME280_TIMEOUT);//xTimerStart(xTimerBME280, 0);
	//while( InitBME280() != BME280_OK ){};	
	//TimerStop(&bme280Timer);			
	//(bme280.delay_ms)(500);	
	///* Continuously stream sensor data */
	//for (;;)
	//{	
		//measureADC();				
		//(bme280.delay_ms)(130);
		///* Wait for the measurement to complete and print data*///rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme280);				
		//TimerStart(&bme280Timer, BME280_TIMEOUT);//xTimerStart(xTimerBME280, 0);
		//while( GetDataBME280() != BME280_OK ){};
		//TimerStop(&bme280Timer);//xTimerStop(xTimerBME280, 0);	
		//if( error_BME280==1 )
		//{						
			//TimerStart(&bme280Timer, BME280_TIMEOUT);//xTimerStart(xTimerBME280, 0);
			//while( InitBME280() != BME280_OK ){};
			//while( GetDataBME280() != BME280_OK ){};
		    //TimerStop(&bme280Timer);//xTimerStop(xTimerBME280, 0);
			//error_BME280=0;		
		//}
	//
		////sensor.Hum1	= comp_data.humidity/1024;
		//if ( len < DATA_LEN && len >= 1) 
		//{
			//if ( comp_data.humidity/1024 <= hum[len-1] )
			//{
				//hum[len] = (float)comp_data.humidity/1024;				
				////sensor.Hum = hum[len];
			//}
			//else
				//hum[len] = hum[len-1];			
			//time[len] = len;
			//len++;
			//
		//}
		//else if ( len == 0)
		//{
			//hum[len] = (float)comp_data.humidity/1024;
			//time[len] = len;
			//len++;
		//}
		//else if ( len == DATA_LEN )
		//{
			////HumidityApproximation((uint8_t*)time, (void*)hum, len, timeExtra);
			//len++;
			//if (( hum[DATA_LEN] < 0 )||( hum[DATA_LEN] > 100.0f )||( hum[DATA_LEN] > hum[DATA_LEN-1] ))
			//{
				//hum[DATA_LEN] = hum[DATA_LEN-1];									
			//}
//
			//sensor.Hum = (uint16_t)hum[DATA_LEN];			
		//}
		//if (hum[DATA_LEN] > (float)comp_data.humidity/1024)
		//{
			//sensor.Hum =  comp_data.humidity/1024;
			//hum[DATA_LEN] = (float)comp_data.humidity/1024;
		//}
								//
		//sensor.Temp	= (float)comp_data.temperature/100;					
		//dewPoint = round(DewPoint(sensor.Temp, hum[DATA_LEN]));
				//
		//if (comp_data.pressure >= 3000000 && comp_data.pressure <= 11000000 && comp_pres >= 30000 && comp_pres <= 110000)
		//{
			//int16_t pressCalc = comp_data.pressure/10000-comp_pres/100;			
			//
			//if (pressCalc < 0 && pressCalc >= -20)
			//{
				//sensor.Press = 0;
			//}
			//else if	(pressCalc >= 200 || pressCalc < -20)
			//{
//#ifdef DEBUG
				//__BKPT();
//#endif				
				//NVIC_SystemReset();
			//}
			//else
				//sensor.Press= pressCalc;			
		//}
		//else if (start<=0)
		//{
//#ifdef DEBUG			
			//__BKPT();
//#endif			
			//NVIC_SystemReset();
		//}
		//
		//if (start==0 )
		//{
			///*if (xAlarmTaskHandle == NULL)
			//{
				//xAlarmTaskHandle = (void*)1;//xTaskCreate(vALARM_Task,(signed char*)"ALARM_Task", configMINIMAL_STACK_SIZE, NULL, 1, &xAlarmTaskHandle);
			//}*/			
			////xTaskCreate(vMOTOR_Task,(signed char*)"MOTOR_Task",	configMINIMAL_STACK_SIZE, NULL, 1, NULL);
			//start--;
		//}		
		//else if (start>0)
		//{
			//sensor.Press=0;					
			//start--;
		//}
//#ifndef DEBUG				
		//WDT_0_reset();		
//#endif		
	//}	
//}

