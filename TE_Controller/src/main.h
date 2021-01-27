/**
 * \file
 *
 * \brief Main functions
 *
 * Copyright (c) 2009-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include "usb_protocol_cdc.h"

#define CURRENT_SENSE_RESISTENCE 0.005f
#define TEC_TEMP_MAX	35.0f
#define TEC_TEMP_MIN	5.0f
#define TEC_TEMP_NORM	25.0f

typedef struct measured_params{
	float	TEC_Temp;
	float	MCU_Temp;
	float	Vin_LFB;
	float	Vin_SFB;
	float	Vin_CS;
	float	DHT22_Temp;
	float	DHT22_Hum;
	float	DS1820_Temp;
	float	TEC_Power;
}measured_params_t;

typedef struct  
{
	float	TEC_Temp;
	float	MCU_Temp;
	float	DHT22_Temp;
	float	DS1820_Temp;
} Temperatures_t;

typedef struct 
{
	float tecV_P;
	float tecV_N;
	float tecI;
} TecState_t;

typedef struct  
{
	Temperatures_t temps;	
	TecState_t tecState;
} Controller_t;

void prvGetRegistersFromStack (uint32_t *pulFaultStackAddress);
void led_configure_port_pins(void);
void vApplicationMallocFailedHook (void);
void vApplicationStackOverflowHook (void);
void Task_cdc_rx(void *parameters);
void Task_cdc_tx(void *parameters);
void Task_led_blink(void *parameters);
void Task_measure(void *parameters);
void Task_regulator(void *parameters);
void Task_backlight(void *parameters);
void InitTask_backlight(void);
void InitTask_cdc_rx(void);
void InitTask_led_blink(void);
void InitTask_regulator(void);
void InitTask_measure(void);
void InitTask_cdc_rx_tx(void);

/*! \brief Opens the communication port
 * This is called by CDC interface when USB Host enable it.
 *
 * \retval true if cdc startup is successfully done
 */
bool main_cdc_enable(uint8_t port);

/*! \brief Closes the communication port
 * This is called by CDC interface when USB Host disable it.
 */
void main_cdc_disable(uint8_t port);

/*! \brief Manages the leds behaviors
 * Called when a start of frame is received on USB line each 1ms.
 */
void main_sof_action(void);

/*! \brief Enters the application in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void main_suspend_action(void);

/*! \brief Turn on a led to notify active mode
 * Called when the USB line is resumed from the suspend state
 */
void main_resume_action(void);

/*! \brief Save new DTR state to change led behavior.
 * The DTR notify that the terminal have open or close the communication port.
 */
void main_cdc_set_dtr(uint8_t port, bool b_enable);

#ifdef USB_DEVICE_LPM_SUPPORT
/*! \brief Enters the application in low power mode
 * Callback called when USB host sets LPM suspend state
 */
void main_suspend_lpm_action(void);

/*! \brief Called by UDC when USB Host request to enable LPM remote wakeup
 */
void main_remotewakeup_lpm_enable(void);

/*! \brief Called by UDC when USB Host request to disable LPM remote wakeup
 */
void main_remotewakeup_lpm_disable(void);
#endif

#endif // _MAIN_H_
