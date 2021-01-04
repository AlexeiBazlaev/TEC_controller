/*
 * backlight.h
 *
 * Created: 10.04.2020 0:41:41
 *  Author: User
 */ 


#ifndef BACKLIGHT_H_
#define BACKLIGHT_H_


void backlight_event_100ms(void);
void backlight_event_1s(void);
void backlight_mode_demo(void);
void backlight_init(void);
void backlight_ws2812_sendarray(void);
void backlight_color_show(uint8_t R, uint8_t G, uint8_t B);

enum backlight_mode
{
	mode_none=0,
	mode_constant,
	mode_demo
};

#endif /* BACKLIGHT_H_ */