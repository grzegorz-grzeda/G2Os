/*================================================================================================*/
/*
 * g2systemInit.h
 *
 *  Created on: 13.10.2016
 *      Author: grzegorz
 */
/*================================================================================================*/
#ifndef G2SYSTEMINIT_H_
#define G2SYSTEMINIT_H_
/*================================================================================================*/
#include "stm32f0xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*================================================================================================*/
#define SYSTICK_RATE_HZ 1000
/*================================================================================================*/
#define OS_THREAD_STACK 200
#define OS_MAX_THREAD_CNT 5
/*================================================================================================*/
typedef struct {
	unsigned int R0;
	unsigned int R1;
	unsigned int R2;
	unsigned int R3;
	unsigned int R12;
	unsigned int LR;
	unsigned int PC;
	unsigned int PSR;
} StosRejestrowPodstawowych;
/*================================================================================================*/
/*================================================================================================*/
void opoznienieMs(unsigned int t);
unsigned int pobierzCzasMs(void);
int zarejestrujPrzerwanie(int irqNumber, void (*irqHandler)(void));
void wyrejestrujPrzerwanie(int irqNumber);
int zarejestrujWatek(void* watek, const char* nazwa, void* parametry);
void uruchomKernel(void);
void wywolajKernel(unsigned int param, void* ptr);
/*================================================================================================*/
/*================================================================================================*/
#endif /* G2SYSTEMINIT_H_ */
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
