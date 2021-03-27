/*
 * eeprom.c
 *
 * Created: 22.02.2021 21:24:14
 *  Author: Lexus
 */ 
#include "eeprom.h"


void configure_eeprom(void)
{
    /* Setup EEPROM emulator service */
    enum status_code error_code = rww_eeprom_emulator_init();
    if (error_code == STATUS_ERR_NO_MEMORY) {
        while (true) {
            /* No EEPROM section has been set in the device's fuses */
        }
    }
    else if (error_code != STATUS_OK) {
        /* Erase the emulated EEPROM memory (assume it is unformatted or
         * irrecoverably corrupt) */
        rww_eeprom_emulator_erase_memory();
        rww_eeprom_emulator_init();
    }
}

enum status_code ReadEEParam(uint16_t *param, uint16_t param_min, uint16_t param_max, uint16_t ee_tag)
{
	uint8_t buf[sizeof(uint16_t)];
	enum status_code ret =	rww_eeprom_emulator_read_buffer(2*ee_tag, buf, sizeof(uint16_t));
	if( ret == STATUS_OK )
	{
		uint16_t ee_data = *(uint16_t*)buf;
		if (param_min <= ee_data && ee_data <= param_max)
		{
			*param = ee_data;
			return ret;
		}
		return STATUS_ERR_INVALID_ARG;
	}
	return ret;	
}

enum status_code WriteEEParam(uint16_t *param_new, uint16_t param_min, uint16_t param_max, uint16_t ee_tag)
{	
	uint16_t param_old = 0xFF;
	enum status_code ret = rww_eeprom_emulator_read_buffer(2*ee_tag, (uint8_t*)&param_old, sizeof(uint16_t));
	if( ret != STATUS_OK ) return ret;	
	if(param_old == *param_new) return STATUS_OK;
	if (param_min <= *param_new && *param_new <= param_max)
	{		
		ret = rww_eeprom_emulator_write_buffer(2*ee_tag, (uint8_t*)param_new, sizeof(uint16_t));		
		if( ret == STATUS_OK )
		{		
			ret = rww_eeprom_emulator_commit_page_buffer();					
		}
		return ret;			
	}
	return STATUS_ERR_INVALID_ARG;
}

/*enum status_code WriteEEParam(double *param, double *param_new, uint16_t param_min, uint16_t param_max, uint16_t ee_tag, size_t size)
{
	if(*param == *param_new) return STATUS_OK;
	if (param_min <= *param_new && *param_new <= param_max)
	{
		enum status_code ret = rww_eeprom_emulator_write_buffer(ee_tag, (uint8_t*)param_new, size);
		if( ret == STATUS_OK )
		{
			ret = rww_eeprom_emulator_commit_page_buffer();
			if( ret == STATUS_OK )
			{
				*param = *param_new;
			}
		}
		return ret;
	}
	return STATUS_ERR_INVALID_ARG;
}*/

#if (SAMD || SAMR21)
void SYSCTRL_Handler(void)
{
    if (SYSCTRL->INTFLAG.reg & SYSCTRL_INTFLAG_BOD33DET) {
        SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33DET;
        rww_eeprom_emulator_commit_page_buffer();
    }
}
#endif
/*static void configure_bod(void)
{
#if (SAMD || SAMR21)
    struct bod_config config_bod33;
    bod_get_config_defaults(&config_bod33);
    config_bod33.action = BOD_ACTION_INTERRUPT;
    // BOD33 threshold level is about 3.2V 
    config_bod33.level = 48;
    bod_set_config(BOD_BOD33, &config_bod33);
    bod_enable(BOD_BOD33);
    SYSCTRL->INTENSET.reg = SYSCTRL_INTENCLR_BOD33DET;
    system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_SYSCTRL);
#endif
}*/