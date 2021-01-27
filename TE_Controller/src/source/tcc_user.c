/*
 * tcc.c
 *
 * Created: 18.01.2021 22:11:12
 *  Author: Lexus
 */ 
#include "tcc_user.h"
#include "stdbool.h"
#include <asf.h>

struct tcc_module tcc_instance;
struct tc_module tc_instance;
bool   tc_callback_flag=false;


void configure_tcc(void)
{
	
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1;
	config_tcc.counter.period = PWM_GCLK_PERIOD;
	
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	
	tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
}

void	print_tcc_status(void){
	uint32_t	tcc_status, tcc_counter;
	uint16_t	ch;
	
	tcc_status = tcc_get_status(&tcc_instance);
	tcc_counter = tcc_get_count_value(&tcc_instance);
	printf("TCC: CNT: %6ld, STATUS: %10ld, SYNC_READY: %d, CAPTURE_OVERFLOW: %d, COUNTER_EVENT: %d, COUNTER_RETRIGGERED: %d, COUNT_OVERFLOW: %d, RAMP_CYCLE_INDEX: %d, STOPPED: %d\r\n", tcc_counter, tcc_status,
	(int)((tcc_status & TCC_STATUS_SYNC_READY)?1:0),
	(int)((tcc_status & TCC_STATUS_CAPTURE_OVERFLOW)?1:0),
	(int)((tcc_status & TCC_STATUS_COUNTER_EVENT)?1:0),
	(int)((tcc_status & TCC_STATUS_COUNTER_RETRIGGERED)?1:0),
	(int)((tcc_status & TCC_STATUS_COUNT_OVERFLOW)?1:0),
	(int)((tcc_status & TCC_STATUS_RAMP_CYCLE_INDEX)?1:0),
	(int)((tcc_status & TCC_STATUS_STOPPED)?1:0)
	);
	for(ch=0;ch<4;ch++){
		printf("channel: %d, CHANNEL_MATCH_CAPTURE: %d, CHANNEL_OUTPUT: %d\r\n", ch,
		(int)((tcc_status & TCC_STATUS_CHANNEL_MATCH_CAPTURE(ch))?1:0),
		(int)((tcc_status & TCC_STATUS_CHANNEL_OUTPUT(ch))?1:0)
		);
	}
}

void configure_tc(void)
{
	struct tc_config config_tc;
	tc_get_config_defaults(&config_tc);
	config_tc.counter_size = TC_COUNTER_SIZE_16BIT;
	config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1024;
	config_tc.counter_16_bit.compare_capture_channel[0] = 4687; // 1000ms
	tc_init(&tc_instance, TC3, &config_tc);
	tc_enable(&tc_instance);
}

void tc_callback(struct tc_module *const module_inst){
	tc_callback_flag=true;
	tc_set_count_value(&tc_instance, 0);
}

void configure_tc_callbacks(void)
{
	tc_register_callback(
	&tc_instance,
	tc_callback,
	TC_CALLBACK_CC_CHANNEL0);
	tc_enable_callback(&tc_instance, TC_CALLBACK_CC_CHANNEL0);
}


