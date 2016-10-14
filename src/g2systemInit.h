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
/*================================================================================================*/
#define SYSTICK_RATE_HZ 1000
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
typedef struct {
	unsigned int R4;
	unsigned int R5;
	unsigned int R6;
	unsigned int R7;
	unsigned int R8;
	unsigned int R9;
	unsigned int R10;
	unsigned int R11;
} StosRejestrowPozostalych;
/*================================================================================================*/
void opoznienieMs(unsigned int t);
unsigned int pobierzCzasMs(void);
int zarejestrujPrzerwanieSystemowe(int irqNumber, void (*irqHandler)(void));
int zarejestrujPrzerwanie(int irqNumber, void (*irqHandler)(void));
void wyrejestrujPrzerwanie(int irqNumber);
void wywolajKernel(unsigned int param, void* ptr);
/*================================================================================================*/
/*================================================================================================*/
#endif /* G2SYSTEMINIT_H_ */
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
