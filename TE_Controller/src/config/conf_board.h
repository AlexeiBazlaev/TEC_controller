/**
 * \file
 *
 * \brief SAM D21 Xplained Pro board configuration.
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
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

#ifndef CONF_BOARD_H_INCLUDED
#define CONF_BOARD_H_INCLUDED

/* Enable USB VBUS detect */
#define CONF_BOARD_USB_VBUS_DETECT
//temp-ctrl-v1.0b
#define	LED_PIN		PIN_PA28
#define LED0_PIN	LED_PIN
#define	TC_TMPGD	PIN_PA01

#define PIN_LNGATE	PIN_PA19
#define PIN_LPGATE	PIN_PA18

#define PIN_SNGATE	PIN_PA17
#define PIN_SPGATE	PIN_PA16

#define CONF_TC_MODULE TC

#define CONF_PWM_MODULE		TCC0
#define CONF_PWM_CHANNEL	0
#define CONF_PWM_OUTPUT		7

#define	PWM_GCLK_PERIOD		42

#define PIN_OW	PIN_PA14

#define PIN_WS2812	PIN_PA23
#define LEN_WS2812	32
//#define LEN_WS2812	2

#define PIN_RS_POWER	PIN_PA00
#define RS_POWER_DEFAULT	true
#define TEC_POWER_DEFAULT	true

#endif /* CONF_BOARD_H_INCLUDED */
