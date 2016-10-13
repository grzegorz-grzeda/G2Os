/*================================================================================================*/
/*
 * g2systemInit.c
 *
 *  Created on: 13.10.2016
 *      Author: grzegorz
 */

/*================================================================================================*/
#include <sys/types.h>
#include <errno.h>
#include "g2systemInit.h"
#include "g2systemLog.h"
/*================================================================================================*/
extern unsigned int __stack;
extern unsigned int __bss_begin;
extern unsigned int __bss_end;
extern unsigned int __data_begin;
extern unsigned int __data_end;
extern unsigned int __data_i_begin;
/*================================================================================================*/
volatile unsigned int systemCnt;
/*================================================================================================*/
void nmi_handler(void);
void hardfault_handler(void) __attribute__ ((naked));
void svc_handler(void);
void pendsv_handler(void);
void systick_handler(void);
void systemInit(void);

extern int main(void);
/*================================================================================================*/
unsigned int * vectorTable[] __attribute__ ((section(".vectorTable"))) = {
      (unsigned int *) &__stack,
      (unsigned int *) systemInit,
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
/*================================================================================================*/
void delayMs(unsigned int t) {
	t += systemCnt;
	while (t != systemCnt)
		;
}
/*================================================================================================*/
unsigned int getSystemTimeMs(void) {
	return systemCnt;
}
/*================================================================================================*/
void systemInit(void) {
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

	SysTick_Config(HSE_VALUE / 1000);
	initLog(115200);
	info("Start systemu.");

	info("Konfigurowanie diod LED.");
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER &= ~(3 << (8 << 1)) | (3 << (9 << 1));
	GPIOC->MODER |= (1 << (8 << 1)) | (1 << (9 << 1));
	GPIOC->ODR = (1 << 9);
	info("System gotowy do pracy.");

	main();
	while (1)
		;
}
/*================================================================================================*/
caddr_t _sbrk(int incr) {
	extern char __heap_begin; // Defined by the linker.
	extern char __heap_end; // Defined by the linker.

	static char* current_heap_end;
	char* current_block_address;

	if (current_heap_end == 0) {
		current_heap_end = &__heap_begin;
	}

	current_block_address = current_heap_end;

	incr = (incr + 3) & (~3); // align value to 4
	if (current_heap_end + incr > &__heap_end) {
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	current_heap_end += incr;
	return (caddr_t) current_block_address;
}
/*================================================================================================*/
void nmi_handler(void) {
	return;
}
/*================================================================================================*/
void hardfault_evaulate(unsigned int *stack) {
	error("TWARDY WYJATEK!");
	error("Wartosci rejestrow rdzenia oraz SCB:");
	printlnHex("R0", stack[0]);
	printlnHex("R1", stack[1]);
	printlnHex("R2", stack[2]);
	printlnHex("R3", stack[3]);
	printlnHex("R12", stack[4]);
	printlnHex("LR", stack[5]);
	printlnHex("PC", stack[6]);
	printlnHex("PSR", stack[7]);
	printlnHex("CPUID", SCB->CPUID);
	printlnHex("ICSR", SCB->ICSR);
	printlnHex("AIRCR", SCB->AIRCR);
	printlnHex("SCR", SCB->SCR);
	printlnHex("CCR", SCB->CCR);
	printlnHex("SHP0", SCB->SHP[0]);
	printlnHex("SHP1", SCB->SHP[1]);
	error("Zatrzymanie systemu...");
	while (1)
		;
}
/*================================================================================================*/
void hardfault_handler(void) {
	__asm( ".syntax unified\n"
			"MOVS   R0, #4  \n"
			"MOV    R1, LR  \n"
			"TST    R0, R1  \n"
			"BEQ    _MSP    \n"
			"MRS    R0, PSP \n"
			"B      hardfault_evaulate \n"
			"_MSP:  \n"
			"MRS    R0, MSP \n"
			"B      hardfault_evaulate \n"
			".syntax divided\n");
}
/*================================================================================================*/
void svc_handler(void) {
	info("Jestem w SVC!");
	return;
}
/*================================================================================================*/
void (*f)(void);
void pendsv_handler(void) {
	GPIOC->ODR |= (3 << 8);
	info("Jestem w PendSV!");

	warning("Wykonuje czynnosc zabroniona!");
	f(); // to spowoduje twardy wyjątek
	while (1)
		;
}
/*================================================================================================*/
void systick_handler(void) {
	systemCnt++;
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
