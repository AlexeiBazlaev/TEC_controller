/*
 * cmd.c
 *
 * Created: 27.01.2021 22:00:17
 *  Author: Lexus
 */ 
#include "cmd.h"
#include "string.h"
#include "eeprom.h"
#include "power_regulator.h"
const char sCmdTotOff[]		= "cmd_turn_off";
const char sCmdTotOn[]		= "cmd_turn_on";

//#ifndef DEBUG
const char sCmdBlReset[]	= "cmd_bl_reset";
const char sCmdBlRgb[]		= "cmd_bl_rgb";
const char sCmdBlRed[]		= "cmd_bl_red";
const char sCmdBlBlue[]		= "cmd_bl_blue";
const char sCmdBlGreen[]	= "cmd_bl_green";
const char sCmdBlYellow[]	= "cmd_bl_yellow";
const char sCmdBlWhite[]	= "cmd_bl_white";
const char sCmdBlDemo[]		= "cmd_bl_demo";
//#endif

const char sCmdRsOn[]		= "cmd_rs_on";
const char sCmdRsOff[]		= "cmd_rs_off";
const char sCmdTecOn[]		= "cmd_tec_on";
const char sCmdTecOff[]		= "cmd_tec_off";
const char sCmdSetTemp[]	= "cmd_set_temp";
const char sCmdSetPow[]		= "cmd_set_pow";
const char sCmdUsbReset[]	= "cmd_usb_reset";

const char sCmdGetData[]	= "cmd_get_data";
const char sCmdTxOn[]		= "cmd_tx_on";
const char sCmdTxOff[]		= "cmd_tx_off";

const char sReplyOk[]		= "OK";
const char sReplyErr[]		= "Error";
const char delim[]			= " ,;=:\n";

extern Controller_t Controller;
CMD_t command = 
{
	.tot_on	= ENABLE,	
	.rs_on	= DISABLE,
	.tec_on	= DISABLE,
	.tx_on	= ENABLE,	
	.get_data	= DISABLE,
	.bl_state	= CMD_BL_RESET,
	.rgb_color.rgb = 0x00000000,	
	.tempSetpoint	= 10
};

__STATIC_INLINE uint32_t ReadRGB(RGB_t *rgb_color)
{
	rgb_color->rgb = 0xFF000000;
	const char *txtVal = strtok(NULL, delim);
	if(txtVal != NULL)
	{		
		long iVal = strtol(txtVal, NULL, 10);
		rgb_color->color.red = (iVal>255)? 255:iVal;
		txtVal = strtok(NULL, delim);
		if(txtVal != NULL)
		{
			iVal = strtol(txtVal, NULL, 10);
			rgb_color->color.green = (iVal>255)? 255:iVal;
			txtVal = strtok(NULL, delim);
			if(txtVal != NULL)
			{
				iVal = strtol(txtVal, NULL, 10);
				rgb_color->color.blue = (iVal>255)? 255:iVal;
				txtVal = strtok(NULL, delim);
				if(txtVal != NULL)
				{
					iVal = strtol(txtVal, NULL, 10);
					rgb_color->color.brightness = (iVal>255)? 255:iVal;
				}			
			}			
		}
	}
	return rgb_color->rgb;
};

__STATIC_INLINE uint32_t ReadBrightness(RGB_t *rgb_color)
{
	rgb_color->color.brightness = 0xFF;
	const char *txtVal = strtok(NULL, delim);
	if(txtVal != NULL)
	{
		long brightness = strtol(txtVal, NULL, 10);	
		rgb_color->color.brightness = (brightness>255)? 255:brightness;		
	}
	return rgb_color->color.brightness;
};

__STATIC_INLINE bool ReadTempValue(float *tempSetpoint)
{
	const char *txtVal = strtok(NULL, delim);
	if(txtVal != NULL)
	{
#ifndef USE_STRTOF		
		uint16_t temp = strtol(txtVal, NULL, 10);
#else
		float temp = strtof(txtVal, NULL);		
#endif				
		if (WriteEEParam(&temp, TEC_TEMP_SETPOINT_MIN, TEC_TEMP_SETPOINT_MAX, EE_TEMP_SETPOINT) == STATUS_OK)
		{
			*tempSetpoint = temp;
			return true;
		}		
	}
	return false;
}

__STATIC_INLINE bool ReadPowerValue(float *powerSetpoint)
{
	const char *txtVal = strtok(NULL, delim);
	if(txtVal != NULL)
	{
		#ifndef USE_STRTOF
		uint16_t power = strtol(txtVal, NULL, 10);
		#else
		float power = strtof(txtVal, NULL);
		#endif		
		if (WriteEEParam(&power, TEC_POWER_COEFF_SETPOINT_MIN, TEC_POWER_COEFF_SETPOINT_MAX, EE_POWER_SETPOINT) == STATUS_OK)
		{
			*powerSetpoint = power;
			return true;
		}		
	}
	return false;
}

__STATIC_INLINE uint8_t ReadFloatValue(float *value, uint16_t valMin, uint16_t valMax, uint16_t ee_tag)
{
	const char *txtVal = strtok(NULL, delim);
	if(txtVal != NULL)
	{
#ifndef USE_STRTOF
		uint16_t val = strtol(txtVal, NULL, 10);
#else
		float val = strtof(txtVal, NULL);
#endif		
		if (WriteEEParam(&val, valMin, valMax, ee_tag) == STATUS_OK)
		{
			*value = val;
			return val;
		}		
	}
	return 254;
}
#ifdef ARM_PID
extern arm_pid_instance_f32  pid;
#else
extern PID_t pid;
#endif
__attribute__((optimize("O3")))
void ProcessCommand(CMD_t *cmd, char* str)
{
	char *token = strtok(str, delim);
	uint8_t brightness = 0;
	uint32_t rgb = 0;
	uint8_t temp = 255;
	uint8_t power = 255;
	while (token != NULL)
	{
		if (strcasecmp(token, sCmdTotOff) == 0)
			cmd->tot_on	= DISABLE;			
		else if (strcasecmp(token, sCmdTotOn) == 0)
			cmd->tot_on	= ENABLE;
		else if (strcasecmp(token, sCmdRsOff) == 0)
			cmd->rs_on	= DISABLE;
		else if (strcasecmp(token, sCmdRsOn) == 0)
			cmd->rs_on	= ENABLE;
		else if (strcasecmp(token, sCmdTecOff) == 0)
		{ 
			PID_Init(&pid); 
			cmd->tec_on	= DISABLE;
			uint16_t tec_state = cmd->tec_on;
			WriteEEParam(&tec_state, DISABLE, ENABLE, EE_TEC_STATE);
		}			
		else if (strcasecmp(token, sCmdTecOn) == 0)
		{ 
			PID_Init(&pid); 
			cmd->tec_on	= ENABLE;
			uint16_t tec_state = cmd->tec_on;
			WriteEEParam(&tec_state, DISABLE, ENABLE, EE_TEC_STATE);
		}
		else if (strcasecmp(token, sCmdSetTemp) == 0)
		{	temp = ReadFloatValue((float*)&Controller.setPoints.tempTecCamSide, TEC_TEMP_SETPOINT_MIN, TEC_TEMP_SETPOINT_MAX, EE_TEMP_SETPOINT);}
		else if (strcasecmp(token, sCmdSetPow) == 0)
		{	power = ReadFloatValue((float*)&Controller.setPoints.powerCoeff, TEC_POWER_COEFF_SETPOINT_MIN, TEC_POWER_COEFF_SETPOINT_MAX, EE_POWER_SETPOINT);}
		/*else if (strcasecmp(token, sCmdSetTemp) == 0 && (temp = ReadTempValue(&Controller.setPoints.tempTecCamSide))){}
		else if (strcasecmp(token, sCmdSetPow) == 0 && (power = ReadPowerValue(&Controller.setPoints.powerCoeff))){}*/
		else if (strcasecmp(token, sCmdTxOff) == 0)
			cmd->tx_on	= DISABLE;
		else if (strcasecmp(token, sCmdTxOn) == 0)
			cmd->tx_on	= ENABLE;
		else if (strcasecmp(token, sCmdGetData) == 0)
			cmd->get_data	= ENABLE;
		else if (strcasecmp(token, sCmdUsbReset) == 0)
			cmd->usb_reset  = ENABLE;
#ifndef DEBUG_REGULATOR			
		else if (strcasecmp(token, sCmdBlReset) == 0)
			cmd->bl_state	= CMD_BL_RESET;			
		else if (strcasecmp(token, sCmdBlRgb) == 0 && (rgb = ReadRGB(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_RGB;			
		else if (strcasecmp(token, sCmdBlRed) == 0 && (brightness = ReadBrightness(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_RED;									
		else if (strcasecmp(token, sCmdBlBlue) == 0 && (brightness = ReadBrightness(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_BLUE;
		else if (strcasecmp(token, sCmdBlGreen) == 0 && (brightness = ReadBrightness(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_GREEN;							
		else if (strcasecmp(token, sCmdBlWhite) == 0 && (brightness = ReadBrightness(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_WHITE;			
		else if (strcasecmp(token, sCmdBlYellow) == 0 && (brightness = ReadBrightness(&cmd->rgb_color)))
			cmd->bl_state	= CMD_BL_YELLOW;		
		else if (strcasecmp(token, sCmdBlDemo) == 0)
			cmd->bl_state	= CMD_BL_DEMO;	
#endif				
		else
		{			
			printf("> %s %s\r\n", token, sReplyErr);			
			token = strtok(NULL, delim);
			continue;
		}
		
		if(brightness || temp < 254 || power < 254)
		{	
			uint16_t param = brightness?brightness:(temp<254?(Controller.setPoints.tempTecCamSide):(Controller.setPoints.powerCoeff));		
			printf("> %s %d %s\r\n", token, (int)param, sReplyOk);
			if (temp || power)		
				PID_Init(&pid);
			brightness = 0;
			temp = power = 255;
		}
		else if(rgb)
		{			
			printf("> %s (R%d, G%d, B%d, Br%d) %s\r\n", token, (int)cmd->rgb_color.color.red&0xFF, (int)cmd->rgb_color.color.green&0xFF, (int)cmd->rgb_color.color.blue&0xFF, (int)cmd->rgb_color.color.brightness&0xFF, sReplyOk);			
			rgb = 0;
		}
		else if (temp == 254 || power == 254)
			printf("> %s %s\r\n", token, sReplyErr);		
		else		
			printf("> %s %s\r\n", token, sReplyOk);				
		token = strtok(NULL, delim);
	}
}