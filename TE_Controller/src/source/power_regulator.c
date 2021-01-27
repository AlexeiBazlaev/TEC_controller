/*
 * power_regulator.c
 *
 * Created: 18.01.2021 22:07:23
 *  Author: Lexus
 */ 
#include "power_regulator.h"

#include <math.h>
#include "tec.h"
struct tcc_module tcc_instance;


void temperature_control(float TEC_Temp, float	*TEC_Power, bool enable)
{	
	if(!enable)
	{		
		TEC_set_level(0);
		return;
	}
	float	temp_goal=10;
	float	temp_diff = (temp_goal - TEC_Temp)*1;
	
	float	TEC_intensity_max=0.5;
	float	TEC_power_calc = *TEC_Power;
	float	K=1.0/10.0;		// 100% power per 20 grad.
	float	Power = K*temp_diff;
	if(Power > 1.0) Power = 1.0;
	if(Power < -1.0) Power = -1.0;
		
	TEC_power_calc = Power*TEC_intensity_max;
	if(fabsf(fabsf(TEC_power_calc) - fabsf(*TEC_Power)) > 0.010)//(isless(TEC_power_calc,*TEC_Power) || isgreater(TEC_power_calc,*TEC_Power))
	{
		*TEC_Power = TEC_power_calc;		
		TEC_set_level(*TEC_Power);
	}	
}

/*void TEC_power_set(bool	value){
	TEC_power = value;
}*/


