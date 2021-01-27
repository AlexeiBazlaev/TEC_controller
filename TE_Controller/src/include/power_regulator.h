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


void temperature_control(float TEC_Temp, float	*TEC_Power, bool enable);
//void TEC_power_set(bool	value);



#endif /* POWER_REGULATOR_H_ */