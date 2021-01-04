/*
 * realsence.c
 *
 * Created: 03.01.2021 22:38:09
 *  Author: Lexus
 */
#include "realsence.h"
bool		rs_power;

void rs_configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(PIN_RS_POWER, &config_port_pin);
}

void rs_set(bool value){
	port_pin_set_output_level(PIN_RS_POWER, value?0:1);
	rs_power = value;
}
