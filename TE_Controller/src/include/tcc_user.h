/*
 * tcc.h
 *
 * Created: 18.01.2021 22:11:39
 *  Author: Lexus
 */ 


#ifndef TCC_H_
#define TCC_H_
#include "tc.h"
void configure_tcc(void);
void print_tcc_status(void);
void configure_tc(void);
void configure_tc_callbacks(void);
void tc_callback(struct tc_module *const module_inst);
#endif /* TCC_H_ */