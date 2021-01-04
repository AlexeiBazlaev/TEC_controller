/*
 * adn8831.c
 *
 * Created: 04.01.2021 19:13:33
 *  Author: Lexus
 */ 
#include "adn8831.h"
#include "ntc.h"
#include "tec.h"
#include "tmpgood.h"

void	make_float2str(char * str, uint16_t len, float	value){
	bool neg_flag;
	if(value >= 0)
	neg_flag = false;
	else{
		neg_flag = true;
		value = value * -1;
	}
	
	int a1 = value;
	int	a2 = (value - a1)*100.0;
	snprintf(str, len, "%c%d.%2.2d",neg_flag?'-':'+', a1, a2);
}

void ADN8831_control(void){
	char	t_TEC_str[MAX_STR_FLOAT];
	char	t_MCU_str[MAX_STR_FLOAT];
	char	v_VTEC_str[MAX_STR_FLOAT];
	char	v_ITEC_str[MAX_STR_FLOAT];
	char	VTEC_str[MAX_STR_FLOAT];
	char	ITEC_str[MAX_STR_FLOAT];
	
	TEC_set_TEMPSET_volt(0.9);
	
	// MCU temp
	float	t = NTC_MCU_get_temp();
	make_float2str(t_MCU_str, MAX_STR_FLOAT, (float)t);
	
	
	// TEC temp
	uint16_t	value_TEC;
	float		R_TEC;
	float		t_TEC = NTC_TEC_get_temp(&value_TEC, &R_TEC);
	make_float2str(t_TEC_str, MAX_STR_FLOAT, (float)t_TEC);
	
	bool		TMPGD = tmpgood_get_state();
	
	// VTEC & ITEC
	float v_VTEC=0, v_ITEC=0, VTEC=0, ITEC=0;
	VTEC_read(&v_VTEC, &VTEC);
	ITEC_read(&v_ITEC, &ITEC);
	
	make_float2str(v_VTEC_str, MAX_STR_FLOAT, v_VTEC);
	make_float2str(v_ITEC_str, MAX_STR_FLOAT, v_ITEC);
	make_float2str(VTEC_str, MAX_STR_FLOAT, VTEC);
	make_float2str(ITEC_str, MAX_STR_FLOAT, ITEC);
	
	printf("Info: t_MCU: %10s, t_TEC:%10s, TMPGD: %d, VTEC: %10s, v_VTEC: %10s, ITEC: %10s, v_ITEC: %10s\r\n",t_MCU_str, t_TEC_str, (int)TMPGD, VTEC_str, v_VTEC_str, ITEC_str, v_ITEC_str);
}