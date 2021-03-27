/*
 * usb_hub.h
 *
 * Created: 13.02.2021 11:59:43
 *  Author: Lexus
 */ 


#ifndef USB_HUB_H_
#define USB_HUB_H_
#include <asf.h>
void usb_hub_init(void);
void usb_hub_reset(uint8_t *enable);
void usb_hub_standby(void);
void usb_hub_normal(void);


#endif /* USB_HUB_H_ */