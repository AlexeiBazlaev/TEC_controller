/*
 * mcu_control.h
 *
 * Created: 12.01.2021 23:17:47
 *  Author: Lexus
 */ 


#ifndef MCU_CONTROL_H_
#define MCU_CONTROL_H_
#include "stdint-gcc.h"
#define MAX_STR_FLOAT	12

void	MCU_control_init(void);
void	MCU_control(void);
float	GetTecCurrent(float v_cs, float v_ref, float r_cs, float gain);

#endif /* MCU_CONTROL_H_ */