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
extern unsigned int __stack;
extern unsigned int __main_stack_size;
extern char __heap_begin;
extern char __heap_end;
/*================================================================================================*/
void systemInit(void);
static void przygotujRam(void);
static void przygotujZegary(void);
static int zapiszPrzerwanie(int irqNumber, void (*irqHandler)(void), unsigned int priorytet);
static void nmi_handler(void);
static void hardfault_handler(void) __attribute__ ((naked));
static void svc_handler(unsigned int param, void* ptr);
static void pendsv_handler(void);
static void systick_handler(void);
static void systemDefault_handler(void);

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
      (unsigned int *) systemDefault_handler, // SVC
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) systemDefault_handler, // PendSV
      (unsigned int *) systemDefault_handler, // SysTick
      (unsigned int *) systemDefault_handler, // WWDG
      (unsigned int *) systemDefault_handler, // PVD
      (unsigned int *) systemDefault_handler, // RTC
      (unsigned int *) systemDefault_handler, // FLASH
      (unsigned int *) systemDefault_handler, // RCC
      (unsigned int *) systemDefault_handler, // EXTI 0 1
      (unsigned int *) systemDefault_handler, // EXTI 2 3
      (unsigned int *) systemDefault_handler, // EXTI 4 15
      (unsigned int *) systemDefault_handler, // TS
      (unsigned int *) systemDefault_handler, // DMA1 1
      (unsigned int *) systemDefault_handler, // DMA1 2 3
      (unsigned int *) systemDefault_handler, // DMA1 4 5
      (unsigned int *) systemDefault_handler, // ADC1 COMP1 COMP2
      (unsigned int *) systemDefault_handler, // TIM1 BRK UP TRIG COM
      (unsigned int *) systemDefault_handler, // TIM1 CC
      (unsigned int *) systemDefault_handler, // TIM2
      (unsigned int *) systemDefault_handler, // TIM3
      (unsigned int *) systemDefault_handler, // TIM6
      (unsigned int *) systemDefault_handler, // TIM14
      (unsigned int *) systemDefault_handler, // TIM15
      (unsigned int *) systemDefault_handler, // TIM16
      (unsigned int *) systemDefault_handler, // TIM17
      (unsigned int *) systemDefault_handler, // I2C1
      (unsigned int *) systemDefault_handler, // I2C2
      (unsigned int *) systemDefault_handler, // SPI1
      (unsigned int *) systemDefault_handler, // SPI2
      (unsigned int *) systemDefault_handler, // USART1
      (unsigned int *) systemDefault_handler, // USART2
      (unsigned int *) systemDefault_handler, // CEC
      (unsigned int *) systemDefault_handler };
/*================================================================================================*/
typedef void (*irqHandlerPtr)(void);
/*================================================================================================*/
static irqHandlerPtr virtualIrqHandlerTable[64];
/*================================================================================================*/
static volatile unsigned int systemCnt;
static struct {
	unsigned int rdzen;
	unsigned int magistralaAHB;
	unsigned int magistralaAPB1;
	unsigned int magistralaAPB2;
} zegaryHz;
/*================================================================================================*/
/*================================================================================================*/
void opoznienieMs(unsigned int t) {
	t += systemCnt;
	while (t != systemCnt)
		;
}
/*================================================================================================*/
unsigned int pobierzCzasMs(void) {
	return systemCnt;
}
/*================================================================================================*/
int zarejestrujPrzerwanie(int irqNumber, void (*irqHandler)(void)) {
	int wynik = zapiszPrzerwanie(irqNumber, irqHandler, 3);
	if ((irqNumber >= 0) && (wynik == 0)) {
		NVIC_EnableIRQ(irqNumber);
	}
	return wynik;
}
/*================================================================================================*/
int zarejestrujPrzerwanieSystemowe(int irqNumber, void (*irqHandler)(void)) {
	unsigned int priorytet = 1;
	if (irqNumber == PendSV_IRQn)
		priorytet = 0;
	return zapiszPrzerwanie(irqNumber, irqHandler, priorytet);
}
/*================================================================================================*/
void wyrejestrujPrzerwanie(int irqNumber) {
	if (irqNumber >= 0)
		NVIC_DisableIRQ(irqNumber);
	virtualIrqHandlerTable[irqNumber + 16] = 0;
}
/*================================================================================================*/
void wywolajKernel(unsigned int param, void* ptr) {
	(void) param;
	(void) ptr;
	__asm("SVC #0\n");
}
/*================================================================================================*/
/*================================================================================================*/
static int zapiszPrzerwanie(int irqNumber, void (*irqHandler)(void), unsigned int priorytet) {
	irqNumber += 16;
	if (virtualIrqHandlerTable[irqNumber])
		return 1;
	else
		virtualIrqHandlerTable[irqNumber] = irqHandler;

	NVIC_SetPriority(irqNumber, priorytet);
	return 0;
}
/*================================================================================================*/
void systemInit(void) {
	int returnCode;

	przygotujRam();
	przygotujZegary();

	zarejestrujPrzerwanieSystemowe(SVC_IRQn, (irqHandlerPtr) svc_handler);
	zarejestrujPrzerwanieSystemowe(PendSV_IRQn, pendsv_handler);
	zarejestrujPrzerwanieSystemowe(SysTick_IRQn, systick_handler);

	SysTick_Config(zegaryHz.rdzen / SYSTICK_RATE_HZ);
	initLog();
	info("--> SYSTEM START <--");
	note("\t   CPU: STM32F051R8T6 @ %d Hz", zegaryHz.rdzen);
	note("\t  Stos: 0x%.8X - %5d B", (unsigned int) &__stack, (unsigned int) &__main_stack_size);
	note("\tSterta: 0x%.8X - %5d B", (unsigned int) &__heap_begin,
	      (unsigned int) (&__heap_end - &__heap_begin));

	returnCode = main();

	if(returnCode)
		warning("System konczy prace z kodem: %d.", returnCode);
	else
		info("System zakonczyl prace bez bledow.");
	info("--> SYSTEM STOP <--");
	while (1)
		;
}
/*================================================================================================*/
static void przygotujRam(void) {
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
}
/*================================================================================================*/
static void przygotujZegary(void) {
	zegaryHz.rdzen = HSE_VALUE;
	zegaryHz.magistralaAHB = HSE_VALUE;
	zegaryHz.magistralaAPB1 = HSE_VALUE;
	zegaryHz.magistralaAPB2 = HSE_VALUE;
}
/*================================================================================================*/
caddr_t _sbrk(int incr) {
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
static void nmi_handler(void) {
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
static void systemDefault_handler(void) {
	unsigned int exceptionNumber = __get_IPSR();
	if (virtualIrqHandlerTable[exceptionNumber] == 0)
		while (1)
			;
	virtualIrqHandlerTable[exceptionNumber]();
}
/*================================================================================================*/
void svc_handler(unsigned int param, void *ptr) {
	info("Jestem w SVC!");
	info("Wydruk przeslanych danych:");
	printlnHex("u32 ", param);
	printlnHex("void*", (unsigned int) ptr);
}
/*================================================================================================*/
static void pendsv_handler(void) {
	info("Jestem w PendSV!");
}
/*================================================================================================*/
static void systick_handler(void) {
	systemCnt++;
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
