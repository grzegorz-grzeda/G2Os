/*================================================================================================*/
/*
 * main.c
 *
 *  Created on: 13.10.2016
 *      Author: grzegorz
 */
/*================================================================================================*/
#include "g2systemInit.h"
#include "g2systemLog.h"
/*================================================================================================*/
void inicjalizuj(void) {
	info("Konfigurowanie diod LED.");
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER &= ~(3 << (8 << 1)) | (3 << (9 << 1));
	GPIOC->MODER |= (1 << (8 << 1)) | (1 << (9 << 1));
	GPIOC->ODR = 0;
	info("Konfigurowanie kernela.");
	info("System gotowy do pracy.");
}
/*================================================================================================*/
void przerzucLed(int numer) {
	numer &= 0xF;
	GPIOC->ODR ^= (1 << numer);
}
/*================================================================================================*/
void watek1(void* param) {
	(void) param;
	if (param) {
		info((const char*) param);
	} else {
		info("Jestem w watku 1.");
	}
	for (;;) {
		przerzucLed(8);
		delayMs(500);
	}
}
;
/*================================================================================================*/
void watek2(void* param) {
	(void) param;
	info("Jestem w watku 2.");
	for (;;) {
		przerzucLed(9);
		delayMs(100);
	}
}
/*================================================================================================*/
int main(void) {
	inicjalizuj();
	registerThread(watek1, "W1", "Witaj swiecie z watka nr 1!");
	registerThread(watek2, "W2", NULL);
	runKernel();
	return 0;
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
