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
extern unsigned int __main_heap_size;
/*================================================================================================*/
void systemInit(void);
static void initRam(void);
static void initClocks(void);
static int saveISR(int irqNumber, void*irqHandler, unsigned int priorytet);
static void NMI_Handler(void);
static void HardFault_Handler(void) __attribute__ ((naked));
static void SVC_Handler(unsigned int param, void* ptr);
static void PendSV_Handler(void) __attribute__ ((naked));
static void SysTick_Handler(void);
static void SystemDefault_Handler(void);

extern int main(void);
/*================================================================================================*/
unsigned int * vectorTable[] __attribute__ ((section(".vectorTable"))) = {
      (unsigned int *) &__stack,
      (unsigned int *) systemInit,
      (unsigned int *) NMI_Handler,
      (unsigned int *) HardFault_Handler,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) SVC_Handler, // SVC
      (unsigned int *) 0,
      (unsigned int *) 0,
      (unsigned int *) PendSV_Handler, // PendSV
      (unsigned int *) SysTick_Handler, // SysTick
      (unsigned int *) SystemDefault_Handler, // WWDG
      (unsigned int *) SystemDefault_Handler, // PVD
      (unsigned int *) SystemDefault_Handler, // RTC
      (unsigned int *) SystemDefault_Handler, // FLASH
      (unsigned int *) SystemDefault_Handler, // RCC
      (unsigned int *) SystemDefault_Handler, // EXTI 0 1
      (unsigned int *) SystemDefault_Handler, // EXTI 2 3
      (unsigned int *) SystemDefault_Handler, // EXTI 4 15
      (unsigned int *) SystemDefault_Handler, // TS
      (unsigned int *) SystemDefault_Handler, // DMA1 1
      (unsigned int *) SystemDefault_Handler, // DMA1 2 3
      (unsigned int *) SystemDefault_Handler, // DMA1 4 5
      (unsigned int *) SystemDefault_Handler, // ADC1 COMP1 COMP2
      (unsigned int *) SystemDefault_Handler, // TIM1 BRK UP TRIG COM
      (unsigned int *) SystemDefault_Handler, // TIM1 CC
      (unsigned int *) SystemDefault_Handler, // TIM2
      (unsigned int *) SystemDefault_Handler, // TIM3
      (unsigned int *) SystemDefault_Handler, // TIM6
      (unsigned int *) SystemDefault_Handler, // TIM14
      (unsigned int *) SystemDefault_Handler, // TIM15
      (unsigned int *) SystemDefault_Handler, // TIM16
      (unsigned int *) SystemDefault_Handler, // TIM17
      (unsigned int *) SystemDefault_Handler, // I2C1
      (unsigned int *) SystemDefault_Handler, // I2C2
      (unsigned int *) SystemDefault_Handler, // SPI1
      (unsigned int *) SystemDefault_Handler, // SPI2
      (unsigned int *) SystemDefault_Handler, // USART1
      (unsigned int *) SystemDefault_Handler, // USART2
      (unsigned int *) SystemDefault_Handler, // CEC
      (unsigned int *) SystemDefault_Handler };
/*================================================================================================*/
typedef struct {
	void (*uchwyt)(void*);
	void *parametry;
	char nazwa[20];
	unsigned int stos[OS_THREAD_STACK];
	unsigned int SP;
} Watek;
/*================================================================================================*/
struct {
	Watek watki[OS_MAX_THREAD_CNT];
	int biezacyWatek;
	int liczbaWatkow;
	int dzialaj;
} OS;
/*================================================================================================*/
typedef void (*irqHandlerPtr)(void);
/*================================================================================================*/
static irqHandlerPtr virtualIrqHandlerTable[64];
/*================================================================================================*/
static volatile unsigned int systemCnt;
static struct {
	unsigned int rdzen;
	unsigned int magistralaAHB;
	unsigned int magistralaAPB;
} zegaryHz;
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
} ExceptionFrame;
/*================================================================================================*/
/*================================================================================================*/
void delayMs(unsigned int t) {
	t += systemCnt;
	while (t != systemCnt)
		;
}
/*================================================================================================*/
unsigned int getTimeMs(void) {
	return systemCnt;
}
/*================================================================================================*/
unsigned int getCoreHz(void) {
	return zegaryHz.rdzen;
}
/*================================================================================================*/
int registerISR(int irqNumber, void* irqHandler) {
	int wynik = saveISR(irqNumber, irqHandler, 3);
	if ((irqNumber >= 0) && (wynik == 0)) {
		NVIC_EnableIRQ(irqNumber);
	}
	return wynik;
}
/*================================================================================================*/
void unregisterISR(int irqNumber) {
	if (irqNumber >= 0)
		NVIC_DisableIRQ(irqNumber);
	virtualIrqHandlerTable[irqNumber + 16] = 0;
}
/*================================================================================================*/
int registerThread(void* watek, const char* nazwa, void* parametry) {
	if (OS.liczbaWatkow == OS_MAX_THREAD_CNT)
		return -1;

	int cnt = OS.liczbaWatkow++;
	Watek* w = &(OS.watki[cnt]);

	w->uchwyt = watek;
	strncpy(w->nazwa, nazwa, 20);
	w->nazwa[19] = 0;
	w->parametry = parametry;

	w->stos[OS_THREAD_STACK - 1] = 0x41000000; // PSR
	w->stos[OS_THREAD_STACK - 2] = (unsigned int) watek; // PC
	w->stos[OS_THREAD_STACK - 8] = (unsigned int) parametry; // R0
	w->stos[OS_THREAD_STACK - 9] = 0xFFFFFFF1; // LR z przerwania

	w->SP = (unsigned int) (&(w->stos[OS_THREAD_STACK - 17]));

	return 0;
}
/*================================================================================================*/
void runKernel(void) {
	OS.biezacyWatek = -1;
	OS.dzialaj = 1;
	while (1)
		;
}
/*================================================================================================*/
void callKernel(unsigned int param, void* ptr) {
	(void) param;
	(void) ptr;
	__asm("SVC #0\n");
}
/*================================================================================================*/
/*================================================================================================*/
static int saveISR(int irqNumber, void*irqHandler, unsigned int priorytet) {
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

	initRam();
	initClocks();

	NVIC_SetPriority(SVC_IRQn, 1);
	NVIC_SetPriority(PendSV_IRQn, 0);
	NVIC_SetPriority(SysTick_IRQn, 1);

	SysTick_Config(zegaryHz.rdzen / SYSTICK_RATE_HZ);
	initLog();
	info("--> SYSTEM START <--");
	note("\t   CPU: STM32F051R8T6 @ %d Hz", zegaryHz.rdzen);
	note("\t  Stos: 0x%.8X - %5d B", (unsigned int) &__stack, (unsigned int) &__main_stack_size);
	note("\tSterta: 0x%.8X - %5d B", (unsigned int) &__heap_begin,
	      (unsigned int) &__main_heap_size);

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
static void initRam(void) {
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
static void initClocks(void) {
	RCC->CR &= ~RCC_CR_PLLON; // PLL off
	while ((RCC->CR & RCC_CR_PLLRDY) != 0)
		;

	RCC->CR |= RCC_CR_HSEON; // HSE on
	while ((RCC->CR & RCC_CR_HSEON) == 0)
		;

	RCC->CFGR = 0x00110000;

	RCC->CR |= RCC_CR_PLLON; // PLL on
	while ((RCC->CR & RCC_CR_PLLRDY) == 0)
		;

	RCC->CFGR |= RCC_CFGR_SW_1;
	RCC->CFGR3 |= 1;

	zegaryHz.rdzen = 48000000;
	zegaryHz.magistralaAHB = 48000000;
	zegaryHz.magistralaAPB = 48000000;
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
static void NMI_Handler(void) {
	return;
}
/*================================================================================================*/
void hardfault_evaulate(ExceptionFrame *r) {
	error("TWARDY WYJATEK!");
	error("Wartosci rejestrow rdzenia oraz SCB:");
	printlnHex("R0", r->R0);
	printlnHex("R1", r->R1);
	printlnHex("R2", r->R2);
	printlnHex("R3", r->R3);
	printlnHex("R12", r->R12);
	printlnHex("LR", r->LR);
	printlnHex("PC", r->PC);
	printlnHex("PSR", r->PSR);
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
void HardFault_Handler(void) {
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
static void SystemDefault_Handler(void) {
	unsigned int exceptionNumber = __get_IPSR();
	if (virtualIrqHandlerTable[exceptionNumber] == 0)
		while (1)
			;
	virtualIrqHandlerTable[exceptionNumber]();
}
/*================================================================================================*/
void SVC_Handler(unsigned int param, void *ptr) {
	info("Jestem w SVC!");
	info("Wydruk przeslanych danych:");
	printlnHex("u32 ", param);
	printlnHex("void*", (unsigned int) ptr);
}
/*================================================================================================*/
static void PendSV_Handler(void) {
	__asm("PUSH {LR} \n"
			"PUSH {R4-R7} \n"
			"MOV R4, R8 \n"
			"MOV R5, R9 \n"
			"MOV R6, R10 \n"
			"MOV R7, R11 \n"
			"PUSH {R4-R7}");
	if (OS.dzialaj) {
		unsigned int stackPtr;
		if (OS.biezacyWatek < 0) {
			OS.biezacyWatek++;
			stackPtr = OS.watki[OS.biezacyWatek].SP;
			__asm volatile("msr msp, %0\n"
					"isb \n"
					"dsb \n"
					::"r"(stackPtr));
		} else {
			__asm volatile("mrs %0, msp\n"
					"isb \n"
					"dsb \n"
					:"=r"(stackPtr));
			OS.watki[OS.biezacyWatek].SP = stackPtr;

			OS.biezacyWatek++;
			OS.biezacyWatek %= OS.liczbaWatkow;

			stackPtr = OS.watki[OS.biezacyWatek].SP;
			__asm volatile("msr msp, %0\n"
					"isb \n"
					"dsb \n"
					::"r"(stackPtr));
		}
	}
	__asm("POP {R4-R7} \n"
			"MOV R11, R7 \n"
			"MOV R10, R6 \n"
			"MOV R9, R5 \n"
			"MOV R8, R4 \n"
			"POP {R4-R7} \n"
			"POP {PC}");
}
/*================================================================================================*/
static void SysTick_Handler(void) {
	__set_PRIMASK(1);
	systemCnt++;
	if ((systemCnt % 10) == 0) {
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
	__set_PRIMASK(0);
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
