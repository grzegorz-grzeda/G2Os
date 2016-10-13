/*================================================================================================*/
/*
 * g2systemLog.c
 *
 *  Created on: 13.10.2016
 *      Author: grzegorz
 */

/*================================================================================================*/
#include "g2systemLog.h"
/*================================================================================================*/
#define RETURN_HOME "\x1B[H"
#define CLEAR_SCREEN "\x1B[2J"
#define DEFAULT_FOREGROUND "\x1B[39m"
#define YELLOW_FOREGROUND "\x1B[33m"
#define RED_FOREGROUND "\x1B[31m"
/*================================================================================================*/
#define LOG_USART USART1
#define LOG_USART_RCC_ENABLE() for(;;){RCC->APB2ENR |= RCC_APB2ENR_USART1EN;break;}
#define LOG_USART_BAUD_DIV (1)
#define LOG_USART_CLOCK (HSE_VALUE)
/*================================================================================================*/
static void printMessage(const char *label, const char* ctrlSeq, const char* text, va_list args);
static void print(const char* text);
static void put(char c);
/*================================================================================================*/
void initLog(unsigned int baudrate) {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~(0x3 << (9 << 1));
	GPIOA->MODER |= (0x2 << (9 << 1));
	GPIOA->AFR[1] &= ~(0x3 << ((9 - 8) << 2));
	GPIOA->AFR[1] |= (0x1 << ((9 - 8) << 2));

	LOG_USART_RCC_ENABLE();
	LOG_USART->BRR = LOG_USART_CLOCK / LOG_USART_BAUD_DIV / baudrate;
	LOG_USART->CR1 = USART_CR1_UE | USART_CR1_TE;

	print(DEFAULT_FOREGROUND);
	print(CLEAR_SCREEN);
	print(RETURN_HOME);
}
/*================================================================================================*/
void info(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("INFO", DEFAULT_FOREGROUND, text, args);
	va_end(args);
}
/*================================================================================================*/
void warning(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("WARNING", YELLOW_FOREGROUND, text, args);
	va_end(args);
}
/*================================================================================================*/
void error(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("ERROR", RED_FOREGROUND, text, args);
	va_end(args);
}
/*================================================================================================*/
void printlnHex(const char* label, unsigned int value) {
	char buffer[30];
	sprintf(buffer, "\t%6s = 0x%.8X\n\r", label, value);
	print(buffer);
}
/*================================================================================================*/
/*================================================================================================*/
static void printMessage(const char *label, const char* ctrlSeq, const char* text, va_list args) {
	char buffer[70];
	print(ctrlSeq);
	sprintf(buffer, "[%8s] - %10d ms - ", label, getSystemTimeMs());
	print(buffer);
	vsprintf(buffer, text, args);
	print(buffer);
	print("\n\r");
}
/*================================================================================================*/
static void print(const char* text) {
	while (*text)
		put(*(text++));
}
/*================================================================================================*/
static void put(char c) {
	while ((LOG_USART->ISR & USART_ISR_TXE) == 0)
		;
	LOG_USART->TDR = c;
}
/*================================================================================================*/
/*                                              EOF                                               */
/*================================================================================================*/
