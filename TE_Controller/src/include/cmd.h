/*
 * cmd.h
 *
 * Created: 27.01.2021 22:00:56
 *  Author: Lexus
 */ 


#ifndef CMD_H_
#define CMD_H_
#include <asf.h>



typedef enum
{
	CMD_OFF = 0,
	CMD_ON,
	CMD_BL_RESET,
	CMD_BL_RED,
	CMD_BL_BLUE,
	CMD_BL_GREEN,
	CMD_BL_YELLOW,
	CMD_BL_WHITE,
	CMD_BL_RGB,
	CMD_BL_DEMO,
			
} CMD;

typedef union
{
	struct  
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t brightness;
	}color;
	uint32_t rgb;
} RGB_t;

typedef struct
{
		uint8_t tot_on;
		uint8_t rs_on;
		uint8_t tec_on;		
		uint8_t tx_on;
		uint8_t get_data;
		uint8_t usb_reset;		
		uint8_t bl_state;
		RGB_t rgb_color;
		float	tempSetpoint;
} CMD_t;
/*typedef union
{
	struct  
	{
		uint32_t cmd_turn_on_off : 1;		
		uint32_t cmd_rs_on_off	 : 1;
		uint32_t cmd_tec_on_off  : 1;
		uint32_t cmd_bl_state	 : 3;
	} bits;
	uint32_t allBits;
} Commands_t;*/
//uint32_t  ReadRGB(RGB_t *rgb_color);
//uint32_t  ReadBrightness(RGB_t *rgb_color);
void ProcessCommand(CMD_t *cmd, char* str);
#endif /* CMD_H_ */