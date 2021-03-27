/*
 * i2c_master.h
 *
 * Created: 06.02.2021 21:02:34
 *  Author: Lexus
 */ 


#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#define DATA_LENGTH 10
/*static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
};*/

//static uint8_t read_buffer[DATA_LENGTH];
//! [packet_data]

//! [address]
#define SLAVE_ADDRESS 0x12
//! [address]

/* Number of times to try to send packet if failed. */
//! [timeout]
#define TIMEOUT 1000
//! [timeout]

void configure_i2c_master(void);



#endif /* I2C_MASTER_H_ */