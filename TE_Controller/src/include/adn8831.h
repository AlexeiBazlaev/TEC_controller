/*
 * adn8831.h
 *
 * Created: 04.01.2021 19:13:57
 *  Author: Lexus
 */ 


#ifndef ADN8831_H_
#define ADN8831_H_
#include <asf.h>
#define MAX_STR_FLOAT	12
void make_float2str(char * str, uint16_t len, float	value);
void ADN8831_control(void);

#endif /* ADN8831_H_ */