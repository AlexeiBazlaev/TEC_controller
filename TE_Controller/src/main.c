/**
 * \file
 *
 * \brief TE_Controller
 *
 * Copyright (c) 2020-2021 Realsence Inc. and its subsidiaries.
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Realsence
 * software and any derivatives exclusively with Realsence products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Realsence software.
 *
 * THIS SOFTWARE IS SUPPLIED BY Realsence "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL Realsence BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 */


//#include <asf.h>
#include <string.h>
#include <arm_math.h>
#include "ui.h"
#include "conf_usb.h"
#include "uart.h"
#include "realsence.h"
#include "ntc.h"
#include "dht22.h"
#include "adc_user.h"
#include "power_regulator.h"
#include "tec.h"
#include "backlight.h"
#include "ws2812.h"
#include "cmd.h"
#include "usb_hub.h"
#include "eeprom.h"


#define PORT_STDIO	0
#define PORT_DATA	1
extern CMD_t command;
uint16_t adc_value = 100;

static volatile bool main_b_cdc_enable = false;
Controller_t Controller;

#ifdef ARM_PID
arm_pid_instance_f32  pid;
#else
PID_t pid;
#ifdef PID_DIFF_CHANGE
TickType_t tempMeasureTick;
#endif
#endif

#define DEBUG_EEPROM
#ifdef DEBUG	
UBaseType_t uxHighWaterMark_cdc_rx, uxHighWaterMark_cdc_tx, uxHighWaterMark_measure,
uxHighWaterMark_regulator, uxHighWaterMark_backlight;
#define GET_STACK_HWM_CDC_RX()    uxHighWaterMark_cdc_rx	= uxTaskGetStackHighWaterMark( NULL );
#define GET_STACK_HWM_CDC_TX()    uxHighWaterMark_cdc_tx	= uxTaskGetStackHighWaterMark( NULL );
#define GET_STACK_HWM_MEASURE()	  uxHighWaterMark_measure   = uxTaskGetStackHighWaterMark( NULL );
#define GET_STACK_HWM_REGULATOR() uxHighWaterMark_regulator = uxTaskGetStackHighWaterMark( NULL );
#define GET_STACK_HWM_BACKLIGHT() uxHighWaterMark_backlight = uxTaskGetStackHighWaterMark( NULL );
#else
#define GET_STACK_HWM_CDC_RX()
#define GET_STACK_HWM_CDC_TX()
#define GET_STACK_HWM_MEASURE()
#define GET_STACK_HWM_REGULATOR()
#define GET_STACK_HWM_BACKLIGHT()
#endif

#ifndef DEBUG_WDT
	#define WDT_RESET() wdt_reset_count();
#else
	#define WDT_RESET()	
#endif
/************************************************************************
* \fn int main(void)
* \brief Main function. Execution starts here.
* \param
* \return
************************************************************************/
int main(void)
{
	irq_initialize_vectors();
	cpu_irq_enable();	
	system_init();	
	InitMCU();
		
	InitTask_cdc_rx_tx();	
	InitTask_measure();
	InitTask_regulator();	
#ifndef DEBUG_REGULATOR	
	InitTask_backlight();
#endif	
	vTaskStartScheduler();
	__BKPT();
}

static void SetDefaultParams(void)
{
	Controller.setPoints.tempTecCamSide = TEC_TEMP_NORM;
	Controller.setPoints.powerCoeff		= TEC_POWER_COEFF_DEFAULT;
	command.tec_on = DISABLE;
}
void ReadEEParameters(void)
{	
	SetDefaultParams();
#ifndef DEBUG_REGULATOR
	uint16_t temp = Controller.setPoints.tempTecCamSide;	
	if(ReadEEParam(&temp, TEC_TEMP_SETPOINT_MIN, TEC_TEMP_SETPOINT_MAX, EE_TEMP_SETPOINT) == STATUS_OK)		
		Controller.setPoints.tempTecCamSide = temp;	
	uint16_t powerCoeff = Controller.setPoints.powerCoeff;
	if(ReadEEParam(&powerCoeff, TEC_POWER_COEFF_SETPOINT_MIN, TEC_POWER_COEFF_SETPOINT_MAX, EE_POWER_SETPOINT) == STATUS_OK)
		Controller.setPoints.powerCoeff = powerCoeff;
	uint16_t tecOn = command.tec_on;		
	if(ReadEEParam(&tecOn, DISABLE, ENABLE, EE_TEC_STATE) == STATUS_OK)
		command.tec_on = tecOn;
#endif		
}
/************************************************************************
* \fn void InitMCU(void)
* \brief
* \param
* \return
************************************************************************/
void InitMCU(void)
{
#ifndef DEBUG_WDT
	struct wdt_conf wdt_config;
	wdt_get_config_defaults(&wdt_config);
	wdt_config.clock_source		= GCLK_GENERATOR_2;		
	if(wdt_set_config(&wdt_config) == STATUS_OK)
		wdt_reset_count();
#endif		
	led_configure_port_pins();
	LED_Off(LED_PIN);	
	configure_eeprom();
	ReadEEParameters();	
	usb_hub_init();			
	rs_configure_port_pins();
	rs_set(ENABLE);
}

/************************************************************************
* \fn void ExecuteCommands(void)
* \brief
* \param
* \return
************************************************************************/
void ExecuteCommands(void)
{
	rs_set(command.rs_on && command.tot_on);
	usb_hub_reset(&command.usb_reset);	
}


/************************************************************************
* \fn void measureADC(void)
* \brief
* \param
* \return
************************************************************************/
void measureADC(MOV_AVG_BUF *movAvgBuf)
{		
	float temp = NTC_MCU_get_temp(NULL);	//Controller.temps.tecCamSide = NTC_MCU_get_temp(NULL);
#ifndef ARM_PID
#ifdef PID_DIFF_CHANGE
	tempMeasureTick = xTaskGetTickCount();
#endif
#endif
	Controller.temps.tecCamSide = GetMovAvg(movAvgBuf, temp, false);
	Controller.temps.tecRadSide = NTC_TEC_get_temp(NULL, NULL);
	Controller.tecState.tecV_N = adc_get_V_spec(chan_SFB)*ADC_MULTIPLIER;
	Controller.tecState.tecV_P = adc_get_V_spec(chan_LFB)*ADC_MULTIPLIER;
	float V_CS = adc_get_V_spec(chan_CS);
	Controller.tecState.tecI   = GetTecCurrent(V_CS, V_REF, R_CS, AMP_GAIN);
}


/************************************************************************
* \fn void Task_measure(void *parameters)
* \brief                                  
* \param
* \return
************************************************************************/
void Task_measure(void *parameters)
{
	uint16_t delayCntr = 10;
	MOV_AVG_BUF tecCamSideTempAvg = {
		.bufFull = 0,
		.idx = 0
	};	
	GET_STACK_HWM_MEASURE()
	for (;;)
	{
		WDT_RESET();		
		if(command.tot_on)
		{						
			measureADC(&tecCamSideTempAvg);		
			if(!delayCntr--)
			{
				measurement_DHT22();
				delayCntr = 1000;			
			}				
		}
		GET_STACK_HWM_MEASURE()
		portYIELD();// vTaskDelay(200);
	}
}

inline void DataLock(void)
{
#ifdef USE_SEMAPHORE	
	xSemaphoreTake(Controller.dataSem,portMAX_DELAY);
#endif	
}

inline void DataUnlock(void)
{
#ifdef USE_SEMAPHORE	
	xSemaphoreGive(Controller.dataSem);
#endif	
}
/************************************************************************
* \fn void Task_regulator(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_regulator(void *parameters)
{				
	PID_Init(&pid);//command.tec_on = CMD_OFF;
#ifndef ARM_PID
	temprChange_t change;
	change.initialized = false;	
#endif
	TEC_Init();				 
	GET_STACK_HWM_REGULATOR();
	for (;;)
	{	
		WDT_RESET();
		DataLock();			
		TempPID_Regulator(command.tec_on && command.tot_on, &Controller, &pid);		
		DataUnlock();	
		GET_STACK_HWM_REGULATOR();
		vTaskDelay(250);
	}
}


/************************************************************************
* \fn void Task_cdc_rx(void *parameters)
* \brief
* \param
* \return
************************************************************************/
#ifndef DEBUG_RX
__attribute__((optimize("Os")))
void Task_cdc_rx(void *parameters)
{
	#define PORT0  0	
	iram_size_t len = 0;
	static size_t totLen = 0;
	static char readBuf[BUF_LEN] = {0};
	static char tmpBuf[BUF_LEN] = {0};
	GET_STACK_HWM_CDC_RX();	
	while(true)
	{
		WDT_RESET();
		if(main_b_cdc_enable &&
		(len = udi_cdc_multi_get_nb_received_data(PORT0)) > 0 &&
		udi_cdc_read_no_polling(tmpBuf, (len<=BUF_LEN)?len:BUF_LEN) > 0)
		{
			totLen += len;
			if(totLen <= BUF_LEN)
			{
				strcat(readBuf, tmpBuf);
				memset(tmpBuf,0,len);
			}
			else
			{
				totLen = BUF_LEN;
				readBuf[totLen-1] = '\r';				
			}
			 						
			if(readBuf[totLen-1] == '\r' || readBuf[totLen-1] == '\n')// || readBuf[totLen-2] == '\r' || readBuf[totLen-2] == '\n')			
			{			
				readBuf[totLen-1]=0;
				DataLock();
				ProcessCommand(&command, readBuf);
				ExecuteCommands();					
				DataUnlock();
				memset(readBuf,0,totLen);
				totLen = 0;
			}
		}
		GET_STACK_HWM_CDC_RX();
	}
}
#else
__attribute__((optimize("Os")))
void Task_cdc_rx(void *parameters)
{
	#define PORT0  0
	iram_size_t len = 0;
	static char readBuf[BUF_LEN] = {0};
	GET_STACK_HWM_CDC_RX();
	while(true)
	{
		WDT_RESET();
		if(main_b_cdc_enable &&
		(len = udi_cdc_multi_get_nb_received_data(PORT0)) >0 &&
		udi_cdc_read_no_polling(readBuf, (len<=BUF_LEN)?len:BUF_LEN) > 0)
		{
			readBuf[len]=0;
			DataLock();
			ProcessCommand(&command, readBuf);
			ExecuteCommands();
			DataUnlock();
		}
		GET_STACK_HWM_CDC_RX();
	}
}
#endif

/************************************************************************
* \fn void Task_cdc_tx(void *parameters)
* \brief
* \param
* \return
************************************************************************/
__attribute__((optimize("Os")))
void Task_cdc_tx(void *parameters)
{
	#define PORT0  0	
	GET_STACK_HWM_CDC_TX();	
	while(true)
	{
		WDT_RESET();
		vTaskDelay(1000);			
		if (main_b_cdc_enable && udi_cdc_multi_is_tx_ready(PORT0) && (command.tx_on || command.get_data) && command.tot_on)
		{
			if(command.get_data) command.get_data = DISABLE;
#ifdef PID_DEBUG
			char string[MAX_STR_FLOAT] ={0,};
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.tecCamSide);
			printf("%s\t", string);						
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.tecState.tecPower*100);
			printf("%s\t", string);
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.dewPoint);
			printf("%s\t", string);
			#ifdef ARM_PID
			make_float2str(string, MAX_STR_FLOAT, pid.A0*pid.state[0]);
			printf("%s\t", string);
			make_float2str(string, MAX_STR_FLOAT, pid.A1*pid.state[1]);
			printf("%s\t", string);
			make_float2str(string, MAX_STR_FLOAT, pid.A2*pid.state[2]);
			printf("%s\r\n", string);
			#else
			make_float2str(string, MAX_STR_FLOAT, (float)pid.var.P);
			printf("%s\t", string);
			make_float2str(string, MAX_STR_FLOAT, (float)pid.var.IVar);
			printf("%s\t", string);
			make_float2str(string, MAX_STR_FLOAT, (float)pid.var.D);
			printf("%s\r\n", string);
			#endif			
#else
			char string[MAX_STR_FLOAT] ={0,};
			if(fpclassify(Controller.temps.tecCamSide) == FP_NAN)
				printf("> Cam_Tmp = NAN\t");
			else
			{
				make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.tecCamSide);
				printf("> Cam_Tmp = %s\t", string);
			}
			if(fpclassify(Controller.temps.tecRadSide) == FP_NAN)
				printf("Rad_Tmp = NAN\t");
			else
			{
				make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.tecRadSide);
				printf("Rad_Tmp = %s\t", string);
			}
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.tecState.tecPower*100);
			printf("TEC_POW = %s\t", string);
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.tecState.tecI);
			printf("TEC_I = %s\t", string);					
			make_float2str(string, MAX_STR_FLOAT, Controller.tecState.tecV_P - Controller.tecState.tecV_N);
			printf("TEC_V = %s\t", string);			
			/*make_float2str(string, MAX_STR_FLOAT, (float)Controller.tecState.tecV_P);
			printf("TEC_V_P = %s,\t", string);
			make_float2str(string, MAX_STR_FLOAT, (float)Controller.tecState.tecV_N);
			printf("TEC_V_N = %s,\t", string);	*/	
			if(fpclassify(Controller.temps.DHT22_Temp) == FP_NAN || fpclassify(Controller.temps.DHT22_Hum) == FP_NAN)
			{
				printf("DHT_Tmp = NAN\t");
				printf("DHT_Hum = NAN\t");
				printf("Dew_Tmp = NAN\t");
			}
			else
			{							
				make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.DHT22_Temp);
				printf("DHT_Tmp = %s\t", string);
				make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.DHT22_Hum);
				printf("DHT_Hum = %s\t", string);
				make_float2str(string, MAX_STR_FLOAT, (float)Controller.temps.dewPoint);
				printf("Dew_Tmp = %s\r\n", string);
			}
#endif
		}				
		GET_STACK_HWM_CDC_TX();
	}
}

/************************************************************************
* \fn void Task_backlight(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_backlight(void *parameters)
{
	uint32_t cnt10 = 0, rgb = 0x00000000;
	uint8_t bl_state = CMD_BL_RESET;	
	GET_STACK_HWM_BACKLIGHT();
	while(1)
	{	
		WDT_RESET();											
		if(!command.tot_on || bl_state == CMD_BL_RESET)
		{
			bl_state = CMD_BL_RESET;
			backlight_color_show(0, 0, 0);			
			vTaskDelay(1000);			
		}		
		if( bl_state != command.bl_state || rgb != command.rgb_color.rgb)
		{
			bl_state = command.bl_state;
			rgb = command.rgb_color.rgb;						
		}		
		else 
		{
			vTaskDelay(200);
			continue;
		}
					
		if(bl_state >= CMD_BL_RESET && bl_state <= CMD_BL_DEMO && command.tot_on)
		{			
			uint8_t brightness = (command.rgb_color.color.brightness)&0xFF;			
			if(bl_state == CMD_BL_RESET)
				backlight_color_show(0, 0, 0);
			else if(bl_state == CMD_BL_RED)			
				backlight_color_show(brightness, 0, 0);			
			else if(bl_state == CMD_BL_GREEN)
				backlight_color_show(0, brightness, 0);		
			else if(bl_state == CMD_BL_BLUE)
				backlight_color_show(0, 0, brightness);
			else if(bl_state == CMD_BL_YELLOW)
			{
				uint8_t red	  = ((brightness > 3)? brightness : 3);
				uint8_t green = ((red       > 3)? brightness/2 : 1);
				backlight_color_show(red, green, 0);
			}
			else if(bl_state == CMD_BL_WHITE)		
				backlight_color_show(brightness, brightness, brightness);			
			else if(bl_state == CMD_BL_RGB)
			{
				uint8_t norm = command.rgb_color.color.red;
				if (command.rgb_color.color.green > norm) norm = command.rgb_color.color.green;
				if (command.rgb_color.color.blue > norm) norm = command.rgb_color.color.blue;
				uint32_t red	= (command.rgb_color.color.red*brightness/norm)&0xFF;				
				uint32_t blue	= (command.rgb_color.color.blue*brightness/norm)&0xFF;
				uint32_t green	= (command.rgb_color.color.green*brightness/norm)&0xFF;
				backlight_color_show(red, green, blue);				
			}
			else if (bl_state == CMD_BL_DEMO)
			{				
				while(command.bl_state == CMD_BL_DEMO)
				{
					backlight_event_100ms();
					vTaskDelay(100);
					if(!((++cnt10)%10))
					backlight_event_1s();
					GET_STACK_HWM_BACKLIGHT();
				}				
			}					
		}				
		GET_STACK_HWM_BACKLIGHT();		
	}
}


/************************************************************************
* \fn void InitTask_cdc_rx_tx(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_cdc_rx_tx(void)
{
	// Enable USB Stack Device	
	stdio_usb_init();
	stdio_usb_enable();	
#ifndef DEBUG_REGULATOR	
	xTaskCreate(Task_cdc_rx, (const char*)"Task_cdc_rx", configMINIMAL_STACK_SIZE*3, NULL,configMAX_PRIORITIES-1, NULL);		
	xTaskCreate(Task_cdc_tx, (const char*)"Task_cdc_tx", configMINIMAL_STACK_SIZE*3, NULL,configMAX_PRIORITIES-1, NULL);
#endif	
}

/************************************************************************
* \fn void InitTask_regulator(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_regulator(void)
{
	xTaskCreate(Task_regulator, (const char*)"Task_regulator", configMINIMAL_STACK_SIZE*3, NULL,configMAX_PRIORITIES-1, NULL);
}

/************************************************************************
* \fn void InitTask_measure(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_measure(void)
{
	configure_adc();
#ifdef USE_SEMAPHORE	
	Controller.dataSem=xSemaphoreCreateMutex();	
#endif	
	xTaskCreate(Task_measure, (const char*)"Task_measure", configMINIMAL_STACK_SIZE*2, NULL,configMAX_PRIORITIES-1, NULL);
#ifdef BME280_TASK
	xTaskCreate(vBME280_task, (const char*)"BME280_task", 5*configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, &pxBME280Task);
#endif
}

/************************************************************************
* \fn void InitTask_backlight(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_backlight(void)
{	
	ws2812_configure_port_pins();//backlight_init();	
	backlight_mode_demo();	
//#ifndef DEBUG_EEPROM	
	xTaskCreate(Task_backlight, (const char*)"Task_backlight", configMINIMAL_STACK_SIZE*1, NULL,configMAX_PRIORITIES-1, NULL);
//#endif	
}

#ifdef DHT22_TASK
UBaseType_t uxHighWaterMark_mesure_DHT22;
/************************************************************************
* \fn void Task_measure_DHT22(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_measure_DHT22(void *parameters)
{
	uxHighWaterMark_mesure_DHT22 = uxTaskGetStackHighWaterMark( NULL );
	for (;;)
	{
		measurement_DHT22();
		uxHighWaterMark_mesure_DHT22 = uxTaskGetStackHighWaterMark( NULL );
		vTaskDelay(2000);
	}
}
/************************************************************************
* \fn void InitTask_measure(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_measure_DHT22(void)
{
	DHT22_EIC_configure();
	xTaskCreate(Task_measure_DHT22, (const char*)"Task_measure_DHT22", configMINIMAL_STACK_SIZE*1+28, NULL,configMAX_PRIORITIES-1, NULL);
}
#endif //DHT22_TASK

#ifdef LED_BLINK_TASK
UBaseType_t uxHighWaterMark_led_blink;
/************************************************************************
* \fn void Task_led_blink(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_led_blink(void *parameters)
{	
	int cnt=0;	
	uxHighWaterMark_led_blink = uxTaskGetStackHighWaterMark( NULL );
	while(1)
	{
		if (main_b_cdc_enable && udi_cdc_multi_is_tx_ready(PORT0))
			printf(">%u sec\n\r", (cnt++));//// stdio_usb_putchar (NULL, "data");//		
		vTaskDelay(1000);
		LED_Toggle(LED_PIN);		
		uxHighWaterMark_led_blink = uxTaskGetStackHighWaterMark( NULL );
	}
}

/************************************************************************
* \fn void Task_led_blink(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void InitTask_led_blink(void)
{	
	led_configure_port_pins();
	LED_Off(LED_PIN);
	xTaskCreate(Task_led_blink, (const char*)"Task_led_blink", configMINIMAL_STACK_SIZE*2, NULL,configMAX_PRIORITIES-1, NULL);
}
#endif //LED_BLINK_TASK

#ifdef BME280_TASK

#include "BME280_task.h"
#include "i2c_master_user.h"
xTaskHandle pxBME280Task;
//xTimerHandle xTimerBME280;
volatile uint8_t error_BME280=0;
uint8_t BME280_ExpireCounter = 5;
portTickType timeoutBME280 = 2000;
Timer_t bme280Timer;
extern sensorData_t sensor;
extern struct bme280_dev bme280;
extern struct bme280_data comp_data;
extern uint32_t comp_pres;
extern int32_t dewPoint;

/************************************************************************
* \fn void vBME280_task(void *parameters)
* \brief
* \param
* \return
************************************************************************/
portTASK_FUNCTION (vBME280_task, pvParameters)
{
	static int8_t rslt __attribute__((used)) = BME280_OK;
	//volatile uint8_t len;
	int8_t start = 2;
	configure_i2c_master();
	TimerStart(&bme280Timer, BME280_TIMEOUT);
	while( InitBME280() != BME280_OK ){};
	TimerStop(&bme280Timer);
	(bme280.delay_ms)(500);
	/* Continuously stream sensor data */
	for (;;)
	{
		measureADC();
		(bme280.delay_ms)(130);
		/* Wait for the measurement to complete and print data*///rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme280);
		TimerStart(&bme280Timer, BME280_TIMEOUT);
		while( GetDataBME280() != BME280_OK ){};
		TimerStop(&bme280Timer);
		if( error_BME280==1 )
		{
			TimerStart(&bme280Timer, BME280_TIMEOUT);
			while( InitBME280() != BME280_OK ){};
			while( GetDataBME280() != BME280_OK ){};
			TimerStop(&bme280Timer);
			error_BME280=0;
		}
		
		sensor.Hum1	= comp_data.humidity/1024;
		sensor.Hum =  comp_data.humidity/1024;
		sensor.Temp	= (float)comp_data.temperature/100;
		dewPoint = round(DewPoint(sensor.Temp, sensor.Hum));
		
		if (comp_data.pressure >= 3000000 && comp_data.pressure <= 11000000 && comp_pres >= 30000 && comp_pres <= 110000)
		{
			int16_t pressCalc = comp_data.pressure/10000-comp_pres/100;
			
			if (pressCalc < 0 && pressCalc >= -20)
			{
				sensor.Press = 0;
			}
			else if	(pressCalc >= 200 || pressCalc < -20)
			{
				#ifdef DEBUG
				__BKPT();
				#endif
				NVIC_SystemReset();
			}
			else
			sensor.Press= pressCalc;
		}
		else if (start<=0)
		{
			#ifdef DEBUG
			__BKPT();
			#endif
			NVIC_SystemReset();
		}
		
		if (start==0 )
		{
			start--;
		}
		else if (start>0)
		{
			sensor.Press=0;
			start--;
		}
		#ifndef DEBUG
		WDT_0_reset();
		#endif
	}
}
/************************************************************************
* \fn void InitTask_BME280(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_BME280(void)
{
	xTaskCreate(vBME280_task, (const char*)"BME280_task", 2*configMINIMAL_STACK_SIZE, NULL, 1, &pxBME280Task);
}

/*! \brief TimerCallback procedure
 *         If xTimer is timed out then this procedure 
 *  \param Not Used.
 */
/*static void vTimerBME280Callback(void)//( xTimerHandle xTimerBME280 )
{		
	if (!error_BME280)
		error_BME280 = true;			

	if (!BME280_ExpireCounter--)		
		NVIC_SystemReset();	
}*/
#endif //BME280_TASK

//#ifdef UI_EXAMPLE

void main_suspend_action(void)
{
	ui_powerdown();
}

void main_resume_action(void)
{
	ui_wakeup();
}

void main_sof_action(void)
{
	if (!main_b_cdc_enable)
		return;
	ui_process(udd_get_frame_number());
}

#ifdef USB_DEVICE_LPM_SUPPORT
void main_suspend_lpm_action(void)
{
	ui_powerdown();
}

void main_remotewakeup_lpm_disable(void)
{
	ui_wakeup_disable();
}

void main_remotewakeup_lpm_enable(void)
{
	ui_wakeup_enable();
}
#endif

bool main_cdc_enable(uint8_t port)
{
	main_b_cdc_enable = true;
	// Open communication
	uart_open(port);
	return true;
}

void main_cdc_disable(uint8_t port)
{
	main_b_cdc_enable = false;
	// Close communication
	uart_close(port);
}


void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if(port == PORT_STDIO){
		if (b_enable) {
			// Host terminal has open COM
			stdio_usb_enable();
			//stdio_cdc_opened=true;
			}else{
			// Host terminal has close COM
			stdio_usb_disable();
			//stdio_cdc_opened=false;
		}
		printf("main_cdc_set_dtr(): %d\r\n", b_enable);
	}
}
//#endif //UI_EXAMPLE
float GetTecCurrent(float v_cs, float v_ref, float r_cs, float gain)
{
	return ((v_cs - v_ref)/(r_cs*gain));
}

void led_configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(LED_PIN, &config_port_pin);
}

void	make_float2str(char * str, uint16_t len, float	value)
{
	bool neg_flag;
	if(value >= 0)
	neg_flag = false;
	else{
		neg_flag = true;
		value = value * -1;
	}
	
	int a1 = value;
	int	a2 = (value - a1)*100.0;
	snprintf(str, len, "%c%02d.%2.2d",neg_flag?'-':'+', a1, a2);
}

float GetMovAvg(MOV_AVG_BUF *buf, float tempr, bool x10)
{	
	float sum;
	if(tempr>=tErrors)
	{
		buf->bufFull=0;
		return tempr;
	}
	if(!buf->bufFull)
	{
		for(uint8_t i=0;i<T_AVG_BUF_SIZE;i++)	
			buf->avgBuf[i]=tempr;
		buf->bufFull=1;
	}
	else
	{
		buf->avgBuf[buf->idx++]=tempr;
		if(buf->idx>=T_AVG_BUF_SIZE)	buf->idx=0;
	}
	
	sum=0;
	for(uint8_t i=0;i<T_AVG_BUF_SIZE;i++)
		sum+=buf->avgBuf[i];
	sum/=T_AVG_BUF_SIZE;
	if(x10)	return (sum+5)/10;
	else return sum;
}


/************************************************************************
* \fn void vApplicationTickHook( void )
* \brief
* \param
* \return
************************************************************************/
void vApplicationTickHook( void );
void vApplicationTickHook( void )
{
	static portTickType delay_1s = 500;
	if(!delay_1s--)
	{
		LED_Toggle(LED_PIN);
		delay_1s = 500;
	}
#ifdef BME280_TASK
	BME280Timer_Handler();
#endif	
}
/************************************************************************
* \fn void vApplicationMallocFailedHook (void)
* \brief
* \param
* \return
************************************************************************/
void vApplicationMallocFailedHook (void) 
{
	#ifdef DEBUG
	__BKPT();
	#endif
	NVIC_SystemReset();
	while (1)
	{
		__BKPT();
	};
}

/************************************************************************
* \fn void vApplicationStackOverflowHook (void)
* \brief
* \param
* \return
************************************************************************/
void vApplicationStackOverflowHook (void) 
{
	#ifdef DEBUG
	__BKPT();
	#endif
	NVIC_SystemReset();
	while (1)
	{
		__BKPT();
	};
}

/************************************************************************
* \fn void prvGetRegistersFromStack (uint32_t *pulFaultStackAddress)
* \brief
* \param
* \return
************************************************************************/
void prvGetRegistersFromStack (uint32_t *pulFaultStackAddress) 
{
	__attribute__((unused)) volatile uint32_t r0;
	__attribute__((unused)) volatile uint32_t r1;
	__attribute__((unused)) volatile uint32_t r2;
	__attribute__((unused)) volatile uint32_t r3;
	__attribute__((unused)) volatile uint32_t r12;
	__attribute__((unused)) volatile uint32_t lr;
	__attribute__((unused)) volatile uint32_t pc;
	__attribute__((unused)) volatile uint32_t psr;

	r0 = pulFaultStackAddress[0];
	r1 = pulFaultStackAddress[1];
	r2 = pulFaultStackAddress[2];
	r3 = pulFaultStackAddress[3];

	r12 = pulFaultStackAddress[4];
	lr = pulFaultStackAddress[5];
	pc = pulFaultStackAddress[6];
	psr = pulFaultStackAddress[7];
	#ifdef DEBUG
	__BKPT();
	#endif
	NVIC_SystemReset();
	while (1)
	{
		__BKPT();
	};
}

/**
 * \mainpage ASF USB Device CDC
 *
 * \section intro Introduction
 * This example shows how to implement a USB Device CDC
 * on Atmel MCU with USB module.
 * The application note AVR4907 http://ww1.microchip.com/downloads/en/appnotes/doc8447.pdf
 * provides more information about this implementation.
 *
 * \section desc Description of the Communication Device Class (CDC)
 * The Communication Device Class (CDC) is a general-purpose way to enable all
 * types of communications on the Universal Serial Bus (USB).
 * This class makes it possible to connect communication devices such as
 * digital telephones or analog modems, as well as networking devices
 * like ADSL or Cable modems.
 * While a CDC device enables the implementation of quite complex devices,
 * it can also be used as a very simple method for communication on the USB.
 * For example, a CDC device can appear as a virtual COM port, which greatly
 * simplifies application development on the host side.
 *
 * \section startup Startup
 * The example is a bridge between a USART from the main MCU
 * and the USB CDC interface.
 *
 * In this example, we will use a PC as a USB host:
 * it connects to the USB and to the USART board connector.
 * - Connect the USART peripheral to the USART interface of the board.
 * - Connect the application to a USB host (e.g. a PC)
 *   with a mini-B (embedded side) to A (PC host side) cable.
 * The application will behave as a virtual COM (see Windows Device Manager).
 * - Open a HyperTerminal on both COM ports (RS232 and Virtual COM)
 * - Select the same configuration for both COM ports up to 115200 baud.
 * - Type a character in one HyperTerminal and it will echo in the other.
 *
 * \note
 * On the first connection of the board on the PC,
 * the operating system will detect a new peripheral:
 * - This will open a new hardware installation window.
 * - Choose "No, not this time" to connect to Windows Update for this installation
 * - click "Next"
 * - When requested by Windows for a driver INF file, select the
 *   atmel_devices_cdc.inf file in the directory indicated in the Atmel Studio
 *   "Solution Explorer" window.
 * - click "Next"
 *
 * \copydoc UI
 *
 * \section example About example
 *
 * The example uses the following module groups:
 * - Basic modules:
 *   Startup, board, clock, interrupt, power management
 * - USB Device stack and CDC modules:
 *   <br>services/usb/
 *   <br>services/usb/udc/
 *   <br>services/usb/class/cdc/
 * - Specific implementation:
 *    - main.c,
 *      <br>initializes clock
 *      <br>initializes interrupt
 *      <br>manages UI
 *      <br>
 *    - uart_xmega.c,
 *      <br>implementation of RS232 bridge for XMEGA parts
 *    - uart_uc3.c,
 *      <br>implementation of RS232 bridge for UC3 parts
 *    - uart_sam.c,
 *      <br>implementation of RS232 bridge for SAM parts
 *    - specific implementation for each target "./examples/product_board/":
 *       - conf_foo.h   configuration of each module
 *       - ui.c        implement of user's interface (leds,buttons...)
 */

/*void Task_cdc_rx(void *parameters)
{
	#define PORT0  0
	char rcvBuf[128];
	char *pStr = rcvBuf;			
	int len=0;	
    uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	
	while(true)
	{			
		if (main_b_cdc_enable)
		{
			int symb = udi_cdc_getc();
			if(symb)
			{
				len += sprintf(pStr++, "%c", symb);				
			}
			if(symb == '\n')
			{
				udi_cdc_write_buf(rcvBuf, len);			
				pStr = rcvBuf;
				len = 0;				
			}
		}		
        uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	}
}*/

/*void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if (b_enable) {
		// Host terminal has open COM
		ui_com_open(port);
	}else{
		// Host terminal has close COM
		ui_com_close(port);
	}
}*/
