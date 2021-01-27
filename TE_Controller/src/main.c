/**
 * \file
 *
 * \brief TE_Controller
 *
 * Copyright (c) 2020-2021 Realsence Inc. and its subsidiaries.
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Realsence
 * software and any derivatives exclusively with Realsence products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Realsence software.
 *
 * THIS SOFTWARE IS SUPPLIED BY Realsence "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL Realsence BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 */


#include <asf.h>
#include "conf_usb.h"
#include "ui.h"
#include "uart.h"
#include <string.h>
//#include <stdlib.h>
#include "realsence.h"
#include "ntc.h"
#include <arm_math.h>
#include "dht.h"
#include "mcu_control.h"
#include "adc_user.h"
#include "power_regulator.h"
#include "tec.h"
#include "backlight.h"
#include "ws2812.h"

#define PORT_STDIO	0
#define PORT_DATA	1

uint16_t adc_value = 100;
//char rcvBuff[128] = {0};
char str[128] = {0};
UBaseType_t uxHighWaterMark_cdc_rx_check, uxHighWaterMark_cdc_tx;
UBaseType_t uxHighWaterMark_led_blink, uxHighWaterMark_mesure, uxHighWaterMark_regulator, uxHighWaterMark_backlight;
static volatile bool main_b_cdc_enable = false;
struct measured_params	m_params;
Controller_t Controller;

/************************************************************************
* \brief Main function. Execution starts here.
************************************************************************/
int main(void)
{
	irq_initialize_vectors();
	cpu_irq_enable();	
	sleepmgr_init();
	system_init();	

	InitTask_cdc_rx_tx();
	InitTask_led_blink();//ui_init();//ui_powerdown();
	InitTask_measure();
	InitTask_regulator();
	InitTask_backlight();
	vTaskStartScheduler();
	__BKPT();
}


/************************************************************************
* \fn void Task_measure(void *parameters)
* \brief                                  
* \param
* \return
************************************************************************/
void Task_measure(void *parameters)
{
	uxHighWaterMark_mesure = uxTaskGetStackHighWaterMark( NULL );
	for (;;)
	{
		Controller.temps.MCU_Temp = NTC_MCU_get_temp(NULL);
		Controller.temps.TEC_Temp = NTC_TEC_get_temp(NULL, NULL);
		//measurement_DHT22();
		Controller.tecState.tecV_N = adc_get_V_spec(chan_LFB);
		Controller.tecState.tecV_P = adc_get_V_spec(chan_SFB);
		Controller.tecState.tecI   = GetTecCurrent(CURRENT_SENSE_RESISTENCE);
				
		uxHighWaterMark_mesure = uxTaskGetStackHighWaterMark( NULL );
		vTaskDelay(20);
	}
}


/************************************************************************
* \fn void Task_regulator(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_regulator(void *parameters)
{	
	uxHighWaterMark_regulator = uxTaskGetStackHighWaterMark( NULL );
	for (;;)
	{
		temperature_control(Controller.temps.MCU_Temp, &m_params.TEC_Power, ENABLE);				
		uxHighWaterMark_regulator = uxTaskGetStackHighWaterMark( NULL );
		//vTaskDelay(1000);
	}
}


/************************************************************************
* \fn void Task_cdc_rx(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_cdc_rx(void *parameters)
{
	#define PORT0  0
	iram_size_t len = 0;
	uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	
	while(true)
	{
		if(main_b_cdc_enable &&
		(len = udi_cdc_multi_get_nb_received_data(PORT0)) > 0 &&
		udi_cdc_read_no_polling(str, (len<=128)?len:128) > 0 )
		{
			str[len]=0;
			printf("<%s\n", str);
		}
		uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	}
}


/************************************************************************
* \fn void Task_cdc_tx(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_cdc_tx(void *parameters)
{
	#define PORT0  0
	uxHighWaterMark_cdc_tx = uxTaskGetStackHighWaterMark( NULL );
	
	while(true)
	{
		if (main_b_cdc_enable && udi_cdc_multi_is_tx_ready(PORT0))
		{
			if(fpclassify(Controller.temps.MCU_Temp) == FP_NAN)
			printf(">NTC_MCU_TEMP = NAN\n\r");
			else
			printf(">NTC_MCU_TEMP = %d\n\r", (int)Controller.temps.MCU_Temp);//printf(">NTC_MCU_TEMP = %d, NTC_TEC_TEMP = %d\n\r", (int)Controller.temps.MCU_Temp, (int)Controller.temps.TEC_Temp);
		}
		LED_Toggle(LED_PIN);
		vTaskDelay(1000);
		uxHighWaterMark_cdc_tx = uxTaskGetStackHighWaterMark( NULL );
	}
}

/************************************************************************
* \fn void Task_led_blink(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_backlight(void *parameters)
{	uint32_t cnt10 = 0;
	uxHighWaterMark_backlight = uxTaskGetStackHighWaterMark( NULL );
	while(1)
	{	
		backlight_color_show(255, 0, 0);
		vTaskDelay(1000);
		backlight_color_show(255, 100, 0);
		vTaskDelay(1000);
		backlight_color_show(0, 255, 0);
		vTaskDelay(1000);
		/*backlight_event_100ms();
		vTaskDelay(100);		
		if(!((++cnt10)%10))
			backlight_event_1s();*/
		uxHighWaterMark_backlight = uxTaskGetStackHighWaterMark( NULL );		
	}
}

/************************************************************************
* \fn void InitTask_cdc_rx_tx(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_cdc_rx_tx(void)
{
	// Enable USB Stack Device	
	stdio_usb_init();//udc_start();		
	stdio_usb_enable();
	
	xTaskCreate(Task_cdc_rx, (const char*)"Task_cdc_rx", configMINIMAL_STACK_SIZE*1, NULL,configMAX_PRIORITIES-1, NULL);
	xTaskCreate(Task_cdc_tx, (const char*)"Task_cdc_tx", configMINIMAL_STACK_SIZE*3, NULL,configMAX_PRIORITIES-1, NULL);
}

/************************************************************************
* \fn void InitTask_regulator(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_regulator(void)
{
	xTaskCreate(Task_regulator, (const char*)"Task_regulator", configMINIMAL_STACK_SIZE*3, NULL,configMAX_PRIORITIES-1, NULL);
}

/************************************************************************
* \fn void InitTask_measure(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_measure(void)
{
	configure_adc();
	xTaskCreate(Task_measure, (const char*)"Task_measure", configMINIMAL_STACK_SIZE*2, NULL,configMAX_PRIORITIES-1, NULL);
}

/************************************************************************
* \fn void InitTask_backlight(void)
* \brief
* \param
* \return
************************************************************************/
void InitTask_backlight(void)
{	
	ws2812_configure_port_pins();
	backlight_init();
	backlight_mode_demo();	
	xTaskCreate(Task_backlight, (const char*)"Task_backlight", configMINIMAL_STACK_SIZE*1, NULL,configMAX_PRIORITIES-1, NULL);
}



/************************************************************************
* \fn void Task_led_blink(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void Task_led_blink(void *parameters)
{	
	int cnt=0;	
	uxHighWaterMark_led_blink = uxTaskGetStackHighWaterMark( NULL );
	while(1)
	{
		if (main_b_cdc_enable && udi_cdc_multi_is_tx_ready(PORT0))
			printf(">%u sec\n\r", (cnt++));//// stdio_usb_putchar (NULL, "data");//		
		vTaskDelay(1000);
		LED_Toggle(LED_PIN);		
		uxHighWaterMark_led_blink = uxTaskGetStackHighWaterMark( NULL );
	}
}

/************************************************************************
* \fn void Task_led_blink(void *parameters)
* \brief
* \param
* \return
************************************************************************/
void InitTask_led_blink(void)
{	
	led_configure_port_pins();
	LED_Off(LED_PIN);
	//xTaskCreate(Task_led_blink, (const char*)"Task_led_blink", configMINIMAL_STACK_SIZE*2, NULL,configMAX_PRIORITIES-1, NULL);
}




void main_suspend_action(void)
{
	ui_powerdown();
}

void main_resume_action(void)
{
	ui_wakeup();
}

void main_sof_action(void)
{
	if (!main_b_cdc_enable)
		return;
	ui_process(udd_get_frame_number());
}

#ifdef USB_DEVICE_LPM_SUPPORT
void main_suspend_lpm_action(void)
{
	ui_powerdown();
}

void main_remotewakeup_lpm_disable(void)
{
	ui_wakeup_disable();
}

void main_remotewakeup_lpm_enable(void)
{
	ui_wakeup_enable();
}
#endif

bool main_cdc_enable(uint8_t port)
{
	main_b_cdc_enable = true;
	// Open communication
	uart_open(port);
	return true;
}

void main_cdc_disable(uint8_t port)
{
	main_b_cdc_enable = false;
	// Close communication
	uart_close(port);
}


void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if(port == PORT_STDIO){
		if (b_enable) {
			// Host terminal has open COM
			stdio_usb_enable();
			//stdio_cdc_opened=true;
			}else{
			// Host terminal has close COM
			stdio_usb_disable();
			//stdio_cdc_opened=false;
		}
		printf("main_cdc_set_dtr(): %d\r\n", b_enable);
	}
}

void led_configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(LED_PIN, &config_port_pin);
}

/************************************************************************
* \fn void vApplicationMallocFailedHook (void)
* \brief
* \param
* \return
************************************************************************/
void vApplicationMallocFailedHook (void) 
{
	while (1)
	{
		__BKPT();
	};
}

/************************************************************************
* \fn void vApplicationStackOverflowHook (void)
* \brief
* \param
* \return
************************************************************************/
void vApplicationStackOverflowHook (void) 
{
	while (1)
	{
		__BKPT();
	};
}

/************************************************************************
* \fn void prvGetRegistersFromStack (uint32_t *pulFaultStackAddress)
* \brief
* \param
* \return
************************************************************************/
void prvGetRegistersFromStack (uint32_t *pulFaultStackAddress) 
{
	__attribute__((unused)) volatile uint32_t r0;
	__attribute__((unused)) volatile uint32_t r1;
	__attribute__((unused)) volatile uint32_t r2;
	__attribute__((unused)) volatile uint32_t r3;
	__attribute__((unused)) volatile uint32_t r12;
	__attribute__((unused)) volatile uint32_t lr;
	__attribute__((unused)) volatile uint32_t pc;
	__attribute__((unused)) volatile uint32_t psr;

	r0 = pulFaultStackAddress[0];
	r1 = pulFaultStackAddress[1];
	r2 = pulFaultStackAddress[2];
	r3 = pulFaultStackAddress[3];

	r12 = pulFaultStackAddress[4];
	lr = pulFaultStackAddress[5];
	pc = pulFaultStackAddress[6];
	psr = pulFaultStackAddress[7];
	#ifdef DEBUG
	__BKPT();
	#endif
	NVIC_SystemReset();
	while (1)
	{
		__BKPT();
	};
}

/**
 * \mainpage ASF USB Device CDC
 *
 * \section intro Introduction
 * This example shows how to implement a USB Device CDC
 * on Atmel MCU with USB module.
 * The application note AVR4907 http://ww1.microchip.com/downloads/en/appnotes/doc8447.pdf
 * provides more information about this implementation.
 *
 * \section desc Description of the Communication Device Class (CDC)
 * The Communication Device Class (CDC) is a general-purpose way to enable all
 * types of communications on the Universal Serial Bus (USB).
 * This class makes it possible to connect communication devices such as
 * digital telephones or analog modems, as well as networking devices
 * like ADSL or Cable modems.
 * While a CDC device enables the implementation of quite complex devices,
 * it can also be used as a very simple method for communication on the USB.
 * For example, a CDC device can appear as a virtual COM port, which greatly
 * simplifies application development on the host side.
 *
 * \section startup Startup
 * The example is a bridge between a USART from the main MCU
 * and the USB CDC interface.
 *
 * In this example, we will use a PC as a USB host:
 * it connects to the USB and to the USART board connector.
 * - Connect the USART peripheral to the USART interface of the board.
 * - Connect the application to a USB host (e.g. a PC)
 *   with a mini-B (embedded side) to A (PC host side) cable.
 * The application will behave as a virtual COM (see Windows Device Manager).
 * - Open a HyperTerminal on both COM ports (RS232 and Virtual COM)
 * - Select the same configuration for both COM ports up to 115200 baud.
 * - Type a character in one HyperTerminal and it will echo in the other.
 *
 * \note
 * On the first connection of the board on the PC,
 * the operating system will detect a new peripheral:
 * - This will open a new hardware installation window.
 * - Choose "No, not this time" to connect to Windows Update for this installation
 * - click "Next"
 * - When requested by Windows for a driver INF file, select the
 *   atmel_devices_cdc.inf file in the directory indicated in the Atmel Studio
 *   "Solution Explorer" window.
 * - click "Next"
 *
 * \copydoc UI
 *
 * \section example About example
 *
 * The example uses the following module groups:
 * - Basic modules:
 *   Startup, board, clock, interrupt, power management
 * - USB Device stack and CDC modules:
 *   <br>services/usb/
 *   <br>services/usb/udc/
 *   <br>services/usb/class/cdc/
 * - Specific implementation:
 *    - main.c,
 *      <br>initializes clock
 *      <br>initializes interrupt
 *      <br>manages UI
 *      <br>
 *    - uart_xmega.c,
 *      <br>implementation of RS232 bridge for XMEGA parts
 *    - uart_uc3.c,
 *      <br>implementation of RS232 bridge for UC3 parts
 *    - uart_sam.c,
 *      <br>implementation of RS232 bridge for SAM parts
 *    - specific implementation for each target "./examples/product_board/":
 *       - conf_foo.h   configuration of each module
 *       - ui.c        implement of user's interface (leds,buttons...)
 */

/*void Task_cdc_rx(void *parameters)
{
	#define PORT0  0
	char rcvBuf[128];
	char *pStr = rcvBuf;			
	int len=0;	
    uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	
	while(true)
	{			
		if (main_b_cdc_enable)
		{
			int symb = udi_cdc_getc();
			if(symb)
			{
				len += sprintf(pStr++, "%c", symb);				
			}
			if(symb == '\n')
			{
				udi_cdc_write_buf(rcvBuf, len);			
				pStr = rcvBuf;
				len = 0;				
			}
		}		
        uxHighWaterMark_cdc_rx_check = uxTaskGetStackHighWaterMark( NULL );
	}
}*/

/*void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if (b_enable) {
		// Host terminal has open COM
		ui_com_open(port);
	}else{
		// Host terminal has close COM
		ui_com_close(port);
	}
}*/
