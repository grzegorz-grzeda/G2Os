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
/*================================================================================================*/
static void NMI_Handler(void);
static void HardFault_Handler(void) __attribute__ ((naked));
static void System_Default_Handler(void);
void Dummy_Handler(void);
void SysTick_Default_Handler(void);
/*================================================================================================*/
void SVC_Handler(unsigned int param, void* ptr);
void PendSV_Handler(void) __attribute__ ((naked, weak, alias("Dummy_Handler")));
void SysTick_Handler(void) __attribute__ ((weak, alias("SysTick_Default_Handler")));
/*================================================================================================*/
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
      (unsigned int *) System_Default_Handler, // WWDG
      (unsigned int *) System_Default_Handler, // PVD
      (unsigned int *) System_Default_Handler, // RTC
      (unsigned int *) System_Default_Handler, // FLASH
      (unsigned int *) System_Default_Handler, // RCC
      (unsigned int *) System_Default_Handler, // EXTI 0 1
      (unsigned int *) System_Default_Handler, // EXTI 2 3
      (unsigned int *) System_Default_Handler, // EXTI 4 15
      (unsigned int *) System_Default_Handler, // TS
      (unsigned int *) System_Default_Handler, // DMA1 1
      (unsigned int *) System_Default_Handler, // DMA1 2 3
      (unsigned int *) System_Default_Handler, // DMA1 4 5
      (unsigned int *) System_Default_Handler, // ADC1 COMP1 COMP2
      (unsigned int *) System_Default_Handler, // TIM1 BRK UP TRIG COM
      (unsigned int *) System_Default_Handler, // TIM1 CC
      (unsigned int *) System_Default_Handler, // TIM2
      (unsigned int *) System_Default_Handler, // TIM3
      (unsigned int *) System_Default_Handler, // TIM6
      (unsigned int *) System_Default_Handler, // TIM14
      (unsigned int *) System_Default_Handler, // TIM15
      (unsigned int *) System_Default_Handler, // TIM16
      (unsigned int *) System_Default_Handler, // TIM17
      (unsigned int *) System_Default_Handler, // I2C1
      (unsigned int *) System_Default_Handler, // I2C2
      (unsigned int *) System_Default_Handler, // SPI1
      (unsigned int *) System_Default_Handler, // SPI2
      (unsigned int *) System_Default_Handler, // USART1
      (unsigned int *) System_Default_Handler, // USART2
      (unsigned int *) System_Default_Handler, // CEC
      (unsigned int *) System_Default_Handler };
/*================================================================================================*/
typedef void (*irqHandlerPtr)(void);
/*================================================================================================*/
static irqHandlerPtr virtualIrqHandlerTable[64];
/*================================================================================================*/
static volatile unsigned int systemCnt;
static struct {
	unsigned int core;
	unsigned int busAHB;
	unsigned int busAPB;
} clocksHz;
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
void delayMs(unsigned int t) {
	t += systemCnt;
	while (t != systemCnt) {
	}
}
/*================================================================================================*/
unsigned int getTimeMs(void) {
	return systemCnt;
}
/*================================================================================================*/
unsigned int getCoreHz(void) {
	return clocksHz.core;
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
	if (irqNumber >= 0) {
		NVIC_DisableIRQ(irqNumber);
	}
	virtualIrqHandlerTable[irqNumber + 16] = 0;
}
/*================================================================================================*/
void callKernel(unsigned int parameter, void* ptr) {
	(void) parameter;
	(void) ptr;
	__asm("SVC #0\n");
}
/*================================================================================================*/
/*================================================================================================*/
static int saveISR(int irqNumber, void*irqHandler, unsigned int priorytet) {
	irqNumber += 16;
	if (virtualIrqHandlerTable[irqNumber]) {
		return 1;
	} else {
		virtualIrqHandlerTable[irqNumber] = irqHandler;
	}

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

	SysTick_Config(clocksHz.core / SYSTICK_RATE_HZ);
	initLog();
	info("--> SYSTEM START <--");
	note("\t   CPU: STM32F051R8T6 @ %d Hz", clocksHz.core);
	note("\t  Stos: 0x%.8X - %5d B", (unsigned int) &__stack, (unsigned int) &__main_stack_size);
	note("\tSterta: 0x%.8X - %5d B", (unsigned int) &__heap_begin,
	      (unsigned int) &__main_heap_size);

	returnCode = main();

	if (returnCode) {
		warning("System konczy prace z kodem: %d.", returnCode);
	} else {
		info("System zakonczyl prace bez bledow.");
	}

	info("--> SYSTEM STOP <--");

	while (1) {
	}
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
	RCC->CR &= ~RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY) != 0) {
	}

	RCC->CR |= RCC_CR_HSEON;
	while ((RCC->CR & RCC_CR_HSEON) == 0) {
	}

	RCC->CFGR = RCC_CFGR_PLLMUL6 | RCC_CFGR_PLLSRC_HSE_PREDIV;

	RCC->CR |= RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
	}

	RCC->CFGR |= RCC_CFGR_SW_PLL;
	RCC->CFGR3 |= RCC_CFGR3_USART1SW_0;

	clocksHz.core = HSE_VALUE * 6;
	clocksHz.busAHB = clocksHz.core;
	clocksHz.busAPB = clocksHz.core;
}
/*================================================================================================*/
caddr_t _sbrk(int incr) {
	static char* current_heap_end;
	char* current_block_address;

	if (current_heap_end == 0) {
		current_heap_end = &__heap_begin;
	}
	current_block_address = current_heap_end;

	incr = (incr + 3) & (~3); // align value to next 4
	if (current_heap_end + incr > &__heap_end) {
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	current_heap_end += incr;
	return (caddr_t) current_block_address;
}
/*================================================================================================*/
static void NMI_Handler(void) {
	error("NON-MASCABLE INTERRUPT!");
	error("Unsupported case.");
	error("System halt...");
	while (1) {
	}
}
/*================================================================================================*/
void hardfault_evaulate(ExceptionFrame *r) {
	error("HARD FAULT!");
	error("Values of Core and SCB registers:");
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
	error("System halt...");
	while (1) {
	}
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
static void System_Default_Handler(void) {
	unsigned int exceptionNumber = __get_IPSR();
	if (virtualIrqHandlerTable[exceptionNumber] == 0) {
		Dummy_Handler();
	} else {
		virtualIrqHandlerTable[exceptionNumber]();
	}
}
/*================================================================================================*/
void SVC_Handler(unsigned int param, void *ptr) {
	info("Jestem w SVC!");
	info("Wydruk przeslanych danych:");
	printlnHex("u32 ", param);
	printlnHex("void*", (unsigned int) ptr);
}
/*================================================================================================*/
void SysTick_Default_Handler(void) {
	__set_PRIMASK(1);
	systemCnt++;
	if ((systemCnt % 10) == 0) {
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
	__set_PRIMASK(0);
}
/*================================================================================================*/
void Dummy_Handler(void) {
	while (1) {
	}
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
