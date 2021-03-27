/*
 * ws2812.c
 *
 * Created: 03.01.2021 23:00:57
 *  Author: Lexus
 */ 
#include "ws2812.h"
#include "conf_board.h"
#include <asf.h>

void ws2812_configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_DOWN;
	port_pin_set_config(PIN_WS2812, &config_port_pin);
}
