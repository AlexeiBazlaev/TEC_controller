/*
 * power_regulator.h
 *
 * Created: 18.01.2021 22:07:56
 *  Author: Lexus
 */ 


#ifndef POWER_REGULATOR_H_
#define POWER_REGULATOR_H_
#include "stdbool.h"
#include <asf.h>
#include "pid_regulator.h"


#define TEC_POWER_COEFF_DEFAULT	40.0F

#define TEC_TEMP_DELTA		0.01F
#define TEMP_SETPOINT_DIFF 1.0F
#define TEMP_SETPOINT_DELTA_OVER_DEWOPINT_MIN 5.0F
#define TEMP_SETPOINT_DELTA_OVER_DEWOPINT_MAX 10.0F
#define TEMP_GRADIENT_LIMIT	70
#ifdef ARM_PID
#define TEC_POWER_DELTA		0.08F	//0.08F
#define COEFF_PROPORTIONAL	0.6F	//0.6F
#define COEFF_INTEGRAL		0.00011F//0.00011F
#define COEFF_DIFFERENTIAL	0.2F	//0.2F	
#else
#define TEC_POWER_DELTA		0.08F//0.1F//0.2F//		
#define COEFF_PROPORTIONAL	0.6F//0.1F//0.3F//		
#define COEFF_INTEGRAL		0.0001F//0.00015F//0.00015F//	
#define COEFF_DIFFERENTIAL	20.0F//60.0F//-5.0F//	

#endif

void temperature_control(Controller_t *Controller, bool enable);
void TempPID_Regulator(bool enable, Controller_t *Controller, void *pid);

#endif /* POWER_REGULATOR_H_ */