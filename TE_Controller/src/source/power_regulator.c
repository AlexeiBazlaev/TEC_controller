/*
 * power_regulator.c
 *
 * Created: 18.01.2021 22:07:23
 *  Author: Lexus
 */ 
#include "power_regulator.h"

#include <math.h>
#include <arm_math.h>
#include "tec.h"


struct tcc_module tcc_instance;

#ifdef ARM_PID
static void PID_Reset_arm(arm_pid_instance_f32 * pid)
{
	pid->Kp = COEFF_PROPORTIONAL,
	pid->Ki = COEFF_INTEGRAL,
	pid->Kd = COEFF_DIFFERENTIAL,
	arm_pid_init_f32(pid,	true);
}

static bool IS_PID_Reset_arm(arm_pid_instance_f32 * pid)
{
	if (fpclassify(pid->state[0]) == FP_ZERO && 
		fpclassify(pid->state[1]) == FP_ZERO && 
		fpclassify(pid->state[2]) == FP_ZERO) return true;
	return false;	
}
#define PID_Reset(pid) PID_Reset_arm((arm_pid_instance_f32*)pid)
#define IS_PID_Reset(pid) IS_PID_Reset_arm((arm_pid_instance_f32*)pid)
#else
static void PID_Reset_zota(PID_t *pid)
{
	pid->Kp = COEFF_PROPORTIONAL,
	pid->Ki = COEFF_INTEGRAL,
	pid->Kd = COEFF_DIFFERENTIAL,
	pid->cntr = 0,
	pid->var.P = 0,
	pid->var.I = 0,
	pid->var.IVar = 0,
	pid->var.D = 0,
	pid->var.power = 0,
	pid->period = 5,
	pid->flag	= 1;
}
static bool IS_PID_Reset_zota(PID_t *pid)
{
	if (!(pid->var.P && pid->var.IVar && pid->var.D)) return true;
	return false;
}
#define PID_Reset(pid) PID_Reset_zota((PID_t*)pid)
#define IS_PID_Reset(pid) IS_PID_Reset_zota((PID_t*)pid)
#endif

inline void PID_Init(void* pid)
{
	PID_Reset(pid);	
}

static inline bool Is_PID_Reset(void* pid)
{
	return (IS_PID_Reset(pid));	
}

static float PID_power(void* pid, Controller_t* Controller, float tempTecCamSide)
{
#ifndef ARM_PID
	float power = PIDRegul(pid, tempTecCamSide, Controller->temps.tecCamSide, &Controller->tecState.tecPower);
#else
	float power = arm_pid_f32(pid, tempTecCamSide - Controller->temps.tecCamSide);	
#endif
	PIDLimitPower(&power,Controller->setPoints.powerCoeff*0.01f,-Controller->setPoints.powerCoeff*0.01f);
	return power;		
}

static inline void getTempSetPoint(float *pTempSetPoint, Controller_t *Controller, void *pid)
{
	float tempSetPoint = *pTempSetPoint;	
	float tempSetPointUser = Controller->setPoints.tempTecCamSide;
	float dewPoint = Controller->temps.dewPoint;
	float tempSetPointMin = Controller->temps.dewPoint + TEMP_SETPOINT_DELTA_OVER_DEWOPINT_MIN;
	float tempSetPointTrigger = dewPoint + TEMP_SETPOINT_DELTA_OVER_DEWOPINT_MAX;
	// Check if user updated temp set point
	if (Is_PID_Reset(pid))
	{
		tempSetPoint = tempSetPointUser;
	}	
	// Check if temp set point is too close to dew point and set it above
	if ( tempSetPoint < tempSetPointMin ||
		 tempSetPoint > tempSetPointTrigger)
	{
		tempSetPoint = tempSetPointMin;		
	}
		
	if (tempSetPoint > TEC_TEMP_SETPOINT_MAX)
		tempSetPoint = TEC_TEMP_SETPOINT_MAX;
	else if (tempSetPoint < tempSetPointUser)
		tempSetPoint = tempSetPointUser;
	
	if ( fabsf(tempSetPoint - *pTempSetPoint) > TEMP_SETPOINT_DIFF)
	{
		*pTempSetPoint = tempSetPoint;
		PID_Init(pid);	
	}	
}

void TempPID_Regulator(bool enable, Controller_t *Controller, void *pid)
{
	static float tempSetPoint = TEC_TEMP_SETPOINT_MIN;		
	if(!enable ||
		fpclassify(Controller->temps.tecCamSide) == FP_NAN ||
		fpclassify(Controller->temps.tecRadSide) == FP_NAN ||
		fabsf(Controller->temps.tecCamSide - Controller->temps.tecRadSide) >= TEMP_GRADIENT_LIMIT)
	{
		if(fpclassify(Controller->tecState.tecPower) != FP_ZERO)
		{
			Controller->tecState.tecToggleCntr++;
			Controller->tecState.tecPower = 0;
			TEC_set_level(0);
		}
		return;
	}
	getTempSetPoint(&tempSetPoint, Controller, pid);
	
	float power = PID_power(pid, Controller, tempSetPoint);			
	if( fabsf(power - Controller->tecState.tecPower) > TEC_POWER_DELTA*Controller->setPoints.powerCoeff*0.01f)
	{
		Controller->tecState.tecPower = power;
		TEC_set_level(Controller->tecState.tecPower);
	}		
}

#ifdef PROPORTIONAL_REGULATOR
void temperature_control(Controller_t *Controller, bool enable)//void temperature_control(float	tempSetpoint, float tecCamSide, float tecRadSide, int *tecToggleCntr, float *TEC_Power, bool enable)
{
	
	if(!enable ||
	fpclassify(Controller->temps.tecCamSide) == FP_NAN ||
	fpclassify(Controller->temps.tecRadSide) == FP_NAN ||
	fabsf((Controller->temps.tecCamSide + 273) - (Controller->temps.tecRadSide  + 273)) >= 70)
	{
		if(fpclassify(Controller->tecState.tecPower) != FP_ZERO)
		{
			Controller->tecState.tecToggleCntr++;
			Controller->tecState.tecPower = 0;
			TEC_set_level(0);
		}
		return;
	}
	float tempSetPoint = Controller->setPoints.tempTecCamSide;
	float TEC_power_calc = Controller->tecState.tecPower;
	#ifdef MINIMIZE_TEC_TOGGLES
	while(true)
	{
		float Power = COEFF_PROPORTIONAL*( tempSetPoint - Controller->temps.tecCamSide);
		if (Power >  1.0)
		Power =  1.0;
		else if (Power < -1.0)
		Power = -1.0;
		
		TEC_power_calc = Power*TEC_POWER_COEFF_DEFAULT*0.01f;
		if(fpclassify(Controller->tecState.tecPower) != FP_ZERO)
		{
			if((TEC_power_calc > 0 && Controller->tecState.tecPower > 0)||
			(TEC_power_calc < 0 && Controller->tecState.tecPower < 0))
			{
				break;
			}
			else if( Controller->temps.tecCamSide <= TEC_TEMP_SETPOINT_MAX &&
			tempSetPoint < Controller->temps.tecCamSide )
			{
				tempSetPoint = Controller->temps.tecCamSide + TEC_TEMP_DELTA;
				continue;
			}
			else if( Controller->temps.tecCamSide >= TEC_TEMP_SETPOINT_MIN &&
			tempSetPoint > Controller->temps.tecCamSide )
			{
				tempSetPoint = Controller->temps.tecCamSide - TEC_TEMP_DELTA;
				continue;
			}
			
			if( fabsf(TEC_power_calc - Controller->tecState.tecPower) > TEC_POWER_DELTA )
			{
				Controller->tecState.tecToggleCntr++;
				if( Controller->temps.tecCamSide > TEC_TEMP_SETPOINT_MAX )
				{
					Controller->setPoints.tempTecCamSide = TEC_TEMP_SETPOINT_MAX;
				}
				else if( Controller->temps.tecCamSide < TEC_TEMP_SETPOINT_MIN )
				{
					Controller->setPoints.tempTecCamSide = TEC_TEMP_SETPOINT_MIN;
				}
			}
		}
		break;
	}
	#else
	float Power = COEFF_PROPORTIONAL*( tempSetPoint - Controller->temps.tecCamSide);
	if (Power >  1.0)
	Power =  1.0;
	else if (Power < -1.0)
	Power = -1.0;
	TEC_power_calc = Power*TEC_POWER_COEFF_DEFAULT*0.01f;
	#endif //MINIMIZE_TEC_TOGGLES
	//if( fabsf(TEC_power_calc - Controller->tecState.tecPower) > TEC_POWER_DELTA )
	{
		Controller->tecState.tecPower = TEC_power_calc;
		TEC_set_level(Controller->tecState.tecPower);
	}
}
#endif

