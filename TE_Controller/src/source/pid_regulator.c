/*
 * pid_regulator.c
 *
 * Created: 02.03.2021 23:04:59
 *  Author: Lexus
 */ 
#include "pid_regulator.h"
#include "arm_math.h"

float errOld;
extern TickType_t tempMeasureTick;
TickType_t tempMeasureTickOld;
extern Controller_t Controller;
/**
*	\fn	static inline void PIDLimitPower(float *power, float up, float down)
*/
inline void PIDLimitPower(float *power, float up, float down)
{
	if(*power > up)
	*power = up;
	else if(*power < down)
	*power = down;
}

/**
*	\fn			static inline void PIDCalcPower(PID *pid, float err, struct TemprChange *change, uint8_t minPower, float maxPower)
*	\brief	Function for calculating power with PID algorithm
*	\param	pid is pointer to PID structure
*	\param	err is the instant temperature error (residual)
*	\param	change is the pointer to the TemprChange structure
*	\param	minPower is the minimum required power set point
*	\param	maxPower is the maximum required power set point
*	\param  index is the PID index
*/
__attribute__((optimize("O0")))
static inline void PIDCalcPower(PID_t *pid, float err, float minPower, float maxPower)
{
	PIDNORM_t pidnorm = {
		.Kpnorm = 1,
		.Kinorm = 1,
		.Kdnorm = 1,
		};
			
	pid->var.P=(float)pid->Kp * err/pidnorm.Kpnorm;
	PIDLimitPower(&pid->var.P,maxPower,minPower);
			
	pid->var.IVar+=(float)pid->Ki * err/pidnorm.Kinorm;
	PIDLimitPower(&pid->var.IVar,maxPower,minPower);
		
	pid->var.D = (float)(pid->Kd * (err - errOld)/pidnorm.Kdnorm)/(tempMeasureTick - tempMeasureTickOld);
	PIDLimitPower(&pid->var.D,maxPower,minPower);
	errOld = err;
	tempMeasureTickOld = tempMeasureTick;
	
	pid->var.power = pid->var.P + pid->var.IVar + pid->var.D;
	PIDLimitPower(&pid->var.power,maxPower,minPower);
}


/**
*	\fn		static inline void DetectTemprChangeRate(float tempr, struct TemprChange *change)
*	\brief	Function detects rate of temperature change
*	\param	tempr is the current temperature
*	\param	change is the pointer to the TemprChange structure
*/
static inline void DetectTemprChangeRate(float tempr, temprChange_t *change)
{
	#define SPEED_FILTER_SAMPLES 5
	#define TIME_SAMPLES		100
	#define DT_CONST			0x7FFF
	#define DTEMP				0.1F
	/*if(tempr >= tErrors)
	{
		change->initialized = FALSE;
		change->time=0x7FFF;
		return;
	}*/
	if(!change->initialized)
	{
		change->initialized	= true;
		change->tempOld		= tempr;
		change->timeCntr	= 0;
		change->filterCntr	= 0;
		change->dt			= DT_CONST;
#ifdef PID_DIFF_CHANGE		
		errOld = Controller.setPoints.tempTecCamSide - tempr;
		tempMeasureTickOld = 0;
#endif		
	}
	else
	{	
		if(change->timeCntr < TIME_SAMPLES)
			change->timeCntr++;
		else
			change->dt = DT_CONST;
		
		if( fabsf(tempr - change->tempOld) >= DTEMP)//if( adsFloat(tempr-change->tOld)>=2)
		{
			if(++change->filterCntr>=SPEED_FILTER_SAMPLES)
			{
				if(tempr > change->tempOld)
					change->dt =  change->timeCntr;
				else
					change->dt = -change->timeCntr;
				change->timeCntr = SPEED_FILTER_SAMPLES-1;
				change->filterCntr = 0;
				change->tempOld=tempr;
			}
		}
		else
		{
			change->filterCntr = 0;
			if(change->timeCntr > abs(change->dt))
			{
				if(change->dt < 0)
					change->dt = -change->timeCntr;
				else
					change->dt = change->timeCntr;
			}
		}
	}	
}

/**
*	\fn		static inline uint8_t PIDRegul(	regulatorParams_t *pRegParam)
*	\brief	Function for calculating PID power
*	\param	pRegParam	is a pointer to regulatorParams_t structure
*	\return	waterPowerPID	is the calculated power
*/
float PIDRegul(PID_t *pid, float tempSetPoint, float temp, float *tec_power)
{		
	PIDCalcPower(pid, (tempSetPoint-temp), -1, 1);						
	return pid->var.power;					
}
