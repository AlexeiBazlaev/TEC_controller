/*
 * dac_user.c
 *
 * Created: 04.01.2021 0:51:07
 *  Author: Lexus
 */ 
#include "dac_user.h"
struct dac_module dac_instance;
void configure_dac_channel(void)
{
	struct dac_chan_config config_dac_chan;
	dac_chan_get_config_defaults(&config_dac_chan);
	dac_chan_set_config(&dac_instance, DAC_CHANNEL_0, &config_dac_chan);
	dac_chan_enable(&dac_instance, DAC_CHANNEL_0);
}