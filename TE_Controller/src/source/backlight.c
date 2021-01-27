/*
 * backlight.c
 *
 * Created: 10.04.2020 0:41:24
 *  Author: User
 */ 

#include <asf.h>
#include <string.h>

#include "light_ws2812_cortex.h"
#include "backlight.h"

enum backlight_mode		mode=mode_none;

bool		mode_run_light=false;
uint8_t		mode_run_light_cnt=0;

uint8_t		led_data[LEN_WS2812*3];


void backlight_event_100ms(void){
	if(mode == mode_demo){
		if(mode_run_light){
			for(uint16_t i=0; i < LEN_WS2812/2;i++){
				if(mode_run_light_cnt == i){
					led_data[0 + i*3] = 0xff;
					led_data[1 + i*3] = 0x00;
					led_data[2 + i*3] = 0x00;

					led_data[0 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0x00;
					led_data[1 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0xff;
					led_data[2 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0x00;
				}else{
					led_data[0 + i*3] = 0x00;
					led_data[1 + i*3] = 0x00;
					led_data[2 + i*3] = 0x00;
				
					led_data[0 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0x00;
					led_data[1 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0x00;
					led_data[2 + (LEN_WS2812/2 - i - 1)*3 + (LEN_WS2812/2)*3] = 0x00;
				}
			}
			backlight_ws2812_sendarray();
			mode_run_light_cnt++;
			if(mode_run_light_cnt >= LEN_WS2812/2){
				mode_run_light=false;
			}
		}		
	}
}

uint32_t	s1_cnt=0;
void backlight_event_1s(void){
	//	LED WS2812
	
	if(mode == mode_demo){
		uint8_t	modes = 6;
		if(!mode_run_light){
			if((s1_cnt % modes) == 0){
				for(uint16_t i=0; i < LEN_WS2812;i++){
					led_data[0 + i*3] = 0xff;
					led_data[1 + i*3] = 0xff;
					led_data[2 + i*3] = 0xff;
				}
			}else if((s1_cnt % modes) == 1){
				for(uint16_t i=0; i < LEN_WS2812;i++){
					led_data[0 + i*3] = 0xff;
					led_data[1 + i*3] = 0x00;
					led_data[2 + i*3] = 0x00;
				}
			}else if((s1_cnt % modes) == 2){
				for(uint16_t i=0; i < LEN_WS2812;i++){
					led_data[0 + i*3] = 0x00;
					led_data[1 + i*3] = 0xff;
					led_data[2 + i*3] = 0x00;
				}
			}else if((s1_cnt % modes) == 3){
				for(uint16_t i=0; i < LEN_WS2812;i++){
					led_data[0 + i*3] = 0x00;
					led_data[1 + i*3] = 0x00;
					led_data[2 + i*3] = 0xff;
				}
			}else if((s1_cnt % modes) == 4){
				for(uint16_t i=0; i < LEN_WS2812;i++){
					led_data[0 + i*3] = 0x00;
					led_data[1 + i*3] = 0x00;
					led_data[2 + i*3] = 0x00;
				}
			}else if((s1_cnt % modes) == 5){
				mode_run_light=true;
				mode_run_light_cnt=0;
			}
			backlight_ws2812_sendarray();
			s1_cnt++;
		}				
	}	
}

void backlight_mode_demo(void){
	mode = mode_demo;
}

void backlight_init(void){
	mode = mode_none;
//	memset(led_data, 0, sizeof(led_data));
	backlight_color_show(0,0,0);
//	for(uint16_t i=0; i < LEN_WS2812*3; i++){
//		printf("%d: %d\r\n", i, (int)led_data[i]);
//	}
}

void backlight_ws2812_sendarray(void){
	portENTER_CRITICAL();//cpu_irq_disable(); 
	ws2812_sendarray(led_data,sizeof(led_data));
	portEXIT_CRITICAL();//cpu_irq_enable();	
}

void backlight_color_show(uint8_t R, uint8_t G, uint8_t B){
	for(uint16_t i=0; i < LEN_WS2812; i++)
	{
		led_data[0 + i*3] = G;
		led_data[1 + i*3] = R;
		led_data[2 + i*3] = B;
//		printf("%d: R: %d, G: %d, B: %d\r\n",(int)i, (int)led_data[1 + i*3], (int)led_data[0 + i*3], (int)led_data[2 + i*3]);
	}
	backlight_ws2812_sendarray();	
}