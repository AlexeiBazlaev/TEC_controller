/*
 * pid_regulator.h
 *
 * Created: 02.03.2021 23:06:14
 *  Author: Lexus
 */ 

#ifndef PID_REGULATOR_H_
#define PID_REGULATOR_H_
#include <asf.h>
#include <arm_math.h>

typedef struct TemprChange
{
	bool initialized;
	uint16_t timeCntr;
	uint8_t filterCntr;
	int16_t dt;	
	float  tempOld;	
}temprChange_t;

typedef struct
{
	float P;
	float I;
	float D;
	float IVar;
	float power;
} pidVar_t;

typedef struct
{
	float Kp;
	float Ki;
	float Kd;
	pidVar_t var;
	uint8_t cntr;
	uint8_t period;
	uint8_t flag;
} PID_t;

typedef struct
{
	uint16_t Kpnorm;
	uint16_t Kinorm;
	uint16_t Kdnorm;
}PIDNORM_t;

float PIDRegul(PID_t *pid, float tempSetPoint, float temp, float *tec_power);
void PID_Init(void *pid);
void PIDLimitPower(float *power, float up, float down);

#endif /* PID_REGULATOR_H_ */