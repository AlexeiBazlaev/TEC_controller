/*
 * tmpgood.c
 *
 * Created: 03.01.2021 23:22:04
 *  Author: Lexus
 */ 
#include "tmpgood.h"
#include "conf_board.h"

void tmpgood_configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(TC_TMPGD, &config_port_pin);
}

bool tmpgood_get_state(void)
{
	return port_pin_get_input_level(TC_TMPGD);
}