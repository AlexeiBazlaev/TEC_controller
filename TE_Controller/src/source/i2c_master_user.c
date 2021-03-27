/*
 * i2c_master.c
 *
 * Created: 06.02.2021 21:01:37
 *  Author: Lexus
 */ 
#include <asf.h>
#include "i2c_master_user.h"
//! [packet_data]


/* Init software module. */
//! [dev_inst]
struct i2c_master_module i2c_master_instance;
//! [dev_inst]


//! [initialize_i2c]
void configure_i2c_master(void)
{
	/* Initialize config structure and software module. */
	//! [init_conf]
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	//! [init_conf]

	/* Change buffer timeout to something longer. */
	//! [conf_change]
	config_i2c_master.buffer_timeout = 10000;
	#if SAMR30
	config_i2c_master.pinmux_pad0    = CONF_MASTER_SDA_PINMUX;
	config_i2c_master.pinmux_pad1    = CONF_MASTER_SCK_PINMUX;
	#endif
	//! [conf_change]
	/* Initialize and enable device with config. */
	//! [init_module]
	i2c_master_init(&i2c_master_instance, CONF_I2C_MASTER_MODULE, &config_i2c_master);
	//! [init_module]

	//! [enable_module]
	i2c_master_enable(&i2c_master_instance);
	//! [enable_module]
}