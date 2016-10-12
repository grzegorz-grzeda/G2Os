#include "stm32f0xx.h"

extern unsigned int __stack;
extern unsigned int __bss_begin;
extern unsigned int __bss_end;
extern unsigned int __data_begin;
extern unsigned int __data_end;
extern unsigned int __data_i_begin;

volatile unsigned int cnt;

void nmi_handler(void);
void hardfault_handler(void);
void svc_handler(void);
void pendsv_handler(void);
void systick_handler(void);

void delay(unsigned int t);
void init(void);
int main(void);

unsigned int * vectorTable[] __attribute__ ((section(".vectorTable"))) = {
      (unsigned int *) &__stack,
      (unsigned int *) main,
      (unsigned int *) nmi_handler,
      (unsigned int *) hardfault_handler,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) svc_handler,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) pendsv_handler,
      (unsigned int *) systick_handler };


void delay(unsigned int t) {
	t += cnt;
	while (t != cnt)
		;
}

void init(void) {
	unsigned int *bss_b = &__bss_begin;
	unsigned int *bss_e = &__bss_end;
	unsigned int *data_b = &__data_begin;
	unsigned int *data_e = &__data_end;
	unsigned int *data_i_b = &__data_i_begin;
	while (bss_b < bss_e) {
		*bss_b = 0;
		bss_b++;
	}
	while (data_b < data_e) {
		*data_b = *data_i_b;
		data_b++;
		data_i_b++;
	}

	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER = (1 << (8 << 1)) | (1 << (9 << 1));
	GPIOC->ODR = (1 << 9);

	SysTick_Config(HSE_VALUE / 1000);
}

int main(void) {
	int i = 0;
	init();
	while (1) {
		delay(500);
		GPIOC->ODR ^= (3 << 8);
		i++;
		if (i == 9) {
			SCB->ICSR |= (1 << 28);
			__DSB();
		}
	}
}
void nmi_handler(void) {
	return;
}
void hardfault_handler(void) {
	return;
}
void svc_handler(void) {
	return;
}
void pendsv_handler(void) {
	GPIOC->ODR |= (3 << 8);
	while (1)
		;
}
void systick_handler(void) {
	cnt++;
}
