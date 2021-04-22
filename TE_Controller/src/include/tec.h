/*
 * tec.h
 *
 * Created: 04.01.2021 17:08:46
 *  Author: Lexus
 */ 


#ifndef TEC_H_
#define TEC_H_
#include <asf.h>

#define PWM_CHANNEL_SNGATE 3
#define PWM_CHANNEL_SPGATE 2
#define PWM_PIN_OUTPUT_SNGATE 7
#define PWM_PIN_OUTPUT_SPGATE 6

bool VTEC_read(float	*p_Vin, float	*p_VTEC);
bool ITEC_read(float	*p_Vin, float	*p_ITEC);
void TEC_L_set(bool	value);
void TEC_set_level(float	value);
void TEC_set_TEMPSET_volt(float	value);
void pin_set_output(uint8_t	pin, bool	output_flag, uint8_t	value);
void TEC_Init(void);
#endif /* TEC_H_ */