#include "stm32f0xx.h"
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

extern unsigned int __stack;
extern unsigned int __bss_begin;
extern unsigned int __bss_end;
extern unsigned int __data_begin;
extern unsigned int __data_end;
extern unsigned int __data_i_begin;

volatile unsigned int systemCnt;

void nmi_handler(void);
void hardfault_handler(void) __attribute__ ((naked));
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

void put(char c) {
	while ((USART1->ISR & USART_ISR_TXE) == 0)
		;
	USART1->TDR = c;
}

void print(const char* text) {
	while (*text)
		put(*(text++));
}

void println(const char* text) {
	print(text);
	print("\n\r");
}
void printlnMemory(const char* label, unsigned int value) {
	char buffer[80];
	sprintf(buffer, "\t%6s = 0x%.8X\n\r", label, value);
	print(buffer);
}
void printMessage(const char *label, const char* text) {
	char buffer[80];
	sprintf(buffer, "[%8s] - %10d ms - %-30s\n\r", label, systemCnt, text);
	print(buffer);
}
void info(const char* text) {
	print("\x1B[39m");
	printMessage("INFO", text);
}
void error(const char* text) {
	print("\x1B[31m");
	printMessage("ERROR", text);
}
void warning(const char* text) {
	print("\x1B[33m");
	printMessage("WARNING", text);
}
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

void delay(unsigned int t) {
	t += systemCnt;
	while (t != systemCnt)
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

	SysTick_Config(HSE_VALUE / 1000);

	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~(0x3 << (9 << 1));
	GPIOA->MODER |= (0x2 << (9 << 1));
	GPIOA->AFR[1] &= ~(0x3 << ((9 - 8) << 2));
	GPIOA->AFR[1] |= (0x1 << ((9 - 8) << 2));

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	USART1->BRR = HSE_VALUE / 115200;
	USART1->CR1 = USART_CR1_UE | USART_CR1_TE;

	print("\x1B[39m");
	print("\x1B[2J");
	print("\x1B[H");
	info("Start systemu.");

	info("Konfigurowanie diod LED.");
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER &= ~(3 << (8 << 1)) | (3 << (9 << 1));
	GPIOC->MODER |= (1 << (8 << 1)) | (1 << (9 << 1));
	GPIOC->ODR = (1 << 9);
	info("System gotowy do pracy.");
}

int main(void) {
	int i = 0;
	init();

	info("Witaj swiecie :)");
	while (1) {
		delay(500);
		GPIOC->ODR ^= (3 << 8);
		i++;
		info("Ku-ku!");
		if (i == 9) {
			info("Wyzwalam zdarzenie PendSV!");
			SCB->ICSR |= (1 << 28);
			__DSB();
		}
	}
}
void nmi_handler(void) {
	return;
}
void hardfault_evaulate(unsigned int *stack) {
	error("TWARDY WYJATEK!");
	error("Wartosci rejestrow rdzenia oraz SCB:");
	printlnMemory("R0", stack[0]);
	printlnMemory("R1", stack[1]);
	printlnMemory("R2", stack[2]);
	printlnMemory("R3", stack[3]);
	printlnMemory("R12", stack[4]);
	printlnMemory("LR", stack[5]);
	printlnMemory("PC", stack[6]);
	printlnMemory("PSR", stack[7]);
	printlnMemory("CPUID", SCB->CPUID);
	printlnMemory("ICSR", SCB->ICSR);
	printlnMemory("AIRCR", SCB->AIRCR);
	printlnMemory("SCR", SCB->SCR);
	printlnMemory("CCR", SCB->CCR);
	printlnMemory("SHP0", SCB->SHP[0]);
	printlnMemory("SHP1", SCB->SHP[1]);
	println(" ");
	error("Zatrzymanie systemu...");
	while (1)
		;
}
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

void svc_handler(void) {
	info("Jestem w SVC!");
	return;
}
void (*f)(void);
void pendsv_handler(void) {
	GPIOC->ODR |= (3 << 8);
	info("Jestem w PendSV!");

	warning("Wykonuje czynnosc zabroniona!");
	// to spowoduje twardy wyjątek
	f();
	while (1)
		;
}
void systick_handler(void) {
	systemCnt++;
}
