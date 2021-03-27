/*
 * eeprom.h
 *
 * Created: 22.02.2021 21:24:49
 *  Author: Lexus
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_
#include <asf.h>

enum EE_TAGS
{
	EE_TEMP_SETPOINT,
	EE_POWER_SETPOINT,	
	EE_TEC_STATE,	
	EE_RS_STATE,	
	EE_FAN_STATE,
	EE_FAN_SPEED,		
};

void configure_eeprom(void);
enum status_code ReadEEParam(uint16_t *param, uint16_t param_min, uint16_t param_max, uint16_t ee_tag);
enum status_code WriteEEParam(uint16_t *param_new, uint16_t param_min, uint16_t param_max, uint16_t ee_tag);
#endif /* EEPROM_H_ */