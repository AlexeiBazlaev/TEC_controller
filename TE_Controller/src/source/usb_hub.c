/*
 * usb_hub.c
 *
 * Created: 13.02.2021 11:58:21
 *  Author: Lexus
 */ 
#include "usb_hub.h"

void usb_hub_init(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PIN_USB_RESET, &config_port_pin);
	port_pin_set_output_level(PIN_USB_RESET, 1);
}


void usb_hub_reset(uint8_t *enable)
{
	if (*enable)
	{
		*enable = 0;
		vTaskDelay(100);		
		port_pin_set_output_level(PIN_USB_RESET, 0);
		vTaskDelay(1);
		port_pin_set_output_level(PIN_USB_RESET, 1);	
	}		
}

void usb_hub_standby(void)
{
	port_pin_set_output_level(PIN_USB_RESET, 0);	
}

void usb_hub_normal(void)
{
	port_pin_set_output_level(PIN_USB_RESET, 1);
}

