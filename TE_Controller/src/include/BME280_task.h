/*
 * BME280_task.h
 *
 * Created: 23.02.2019 9:05:31
 *  Author: Lexus
 */ 


#ifndef BME280_TASK_H_
#define BME280_TASK_H_
#include "bme280_defs.h"

#define BME280_MEM_ADDR 0
#define BME280_BUS_ADDR 0x76
#define BMP280_BUS_ADDR 0x77

#define BME280_TIMEOUT 2000
#define DATA_LEN 30

typedef struct
{
	TickType_t timeStart;
	TickType_t timeout;
} Timer_t;

typedef struct
{
	uint16_t Press;
	uint16_t Hum;
	uint16_t Hum1;
	float  Temp;
}sensorData_t;

void TimerStart(Timer_t *timer, TickType_t timeout_ms);
void TimerStop(Timer_t *timer);
bool IsTimeOut(Timer_t *timer);
void BME280Timer_Handler(void);
int8_t GetDataBME280(void);
int8_t InitBME280(void);
float DewPoint(float temp, float hum);
//void InitTask_BME280(void *parameters);
//void vStart_BME280_Task( unsigned portBASE_TYPE uxPriority );
portTASK_FUNCTION_PROTO (vBME280_task, pvParameters);



#endif /* BME280_TASK_H_ */