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
#define LICZBA_PROB 5
#define WYWOLANIE_DLA_SVC 2
#define WYWOLANIE_DLA_PENDSV 3
#define WYWOLANIE_DLA_HARDFAULT 4
/*================================================================================================*/
void inicjalizuj(void) {
	info("Konfigurowanie diod LED.");
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER &= ~(3 << (8 << 1)) | (3 << (9 << 1));
	GPIOC->MODER |= (1 << (8 << 1)) | (1 << (9 << 1));
	GPIOC->ODR = (1 << 9);
	info("System gotowy do pracy.");
}
/*================================================================================================*/
void przerzucLed(int numer) {
	numer &= 0xF;
	GPIOC->ODR ^= (1 << numer);
}
/*================================================================================================*/
void wywolajPendSV(void) {
	SCB->ICSR |= (1 << 28);
}
/*================================================================================================*/
void wykrzaczSystem(void) {
	unsigned int *slowo32 = (unsigned int *) 0x08000000;
	warning("Probuje zapisac do FLASH przez zwykly wskaznik...");
	*slowo32 = 1;
	(void) slowo32;
}
/*================================================================================================*/
int main(void) {
	int kodBledu = (GPIOA->IDR & 1) ? 1 : 0;

	inicjalizuj();

	for (int i = 0; i < LICZBA_PROB; i++) {
		info("Ku-ku po raz %d!", i);
		przerzucLed(8);
		przerzucLed(9);

		switch (i) {
			case WYWOLANIE_DLA_SVC:
				info("Wywoluje funkcje kernela!");
				wywolajKernel(kodBledu, &kodBledu);
				break;
			case WYWOLANIE_DLA_PENDSV:
				info("Wyzwalam zdarzenie PendSV!");
				wywolajPendSV();
				break;
			case WYWOLANIE_DLA_HARDFAULT:
				if (GPIOA->IDR & 1)
					wykrzaczSystem();
				break;
			default:
				break;
		}

		opoznienieMs(500);
	}
	return kodBledu;
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
