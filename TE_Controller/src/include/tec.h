/*
 * tec.h
 *
 * Created: 04.01.2021 17:08:46
 *  Author: Lexus
 */ 


#ifndef TEC_H_
#define TEC_H_
#include <asf.h>

bool VTEC_read(float	*p_Vin, float	*p_VTEC);
bool ITEC_read(float	*p_Vin, float	*p_ITEC);
void TEC_L_set(bool	value);
void TEC_set_level(float	value);
void TEC_set_TEMPSET_volt(float	value);
void pin_set_output(uint8_t	pin, bool	output_flag, uint8_t	value);
#endif /* TEC_H_ */