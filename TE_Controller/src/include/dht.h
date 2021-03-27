/*
 * dht.h
 *
 * Created: 04.01.2021 21:06:23
 *  Author: Lexus
 */ 


#ifndef DHT_H_
#define DHT_H_

#include <asf.h>

/* ��������� ������������ �������� ������ */
typedef struct {
	float hum;
	float temp;
} DHT_data;
/* ��� ������������� ������� */
typedef enum {
	DHT11,
	DHT22
} DHT_type;

/* ��������� */
#define DHT_Pin 	PIN_PA15 			//��� ����� ������
#define DHT_timeout 100000			//���������� ��������, ����� ������� ������� ����� ������ ��������

/* ��������� ������� */
uint8_t DHT_getData(DHT_type t, DHT_data*	p_data); //�������� ������ � �������
uint8_t _DHT_getData(DHT_type t, DHT_data*	p_data);
void	measurement_DHT22(void);



#endif /* DHT_H_ */