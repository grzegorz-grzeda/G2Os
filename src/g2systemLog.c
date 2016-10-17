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
#define DEFAULT_COLOR "\x1B[0;39;49m"
#define YELLOW_COLOR "\x1B[33;49m"
#define RED_COLOR "\x1B[31;49m"
#define CRITICAL_COLOR "\x1B[31;46m"
/*================================================================================================*/
#define LOG_USART USART1
#define LOG_USART_RCC_ENABLE() for(;;){RCC->APB2ENR |= RCC_APB2ENR_USART1EN;break;}
#define LOG_USART_BAUD_DIV (1)
/*================================================================================================*/
static void initializeGPIO(void);
static void initializeUSART(void);
static void printInitialText(void);
static void printMessage(const char *label, const char* ctrlSeq, const char* text, va_list args);
static void print(const char* text);
static void put(char c);
/*================================================================================================*/
void initLog(void) {
	initializeGPIO();
	initializeUSART();
	printInitialText();
}
/*================================================================================================*/
static void initializeGPIO(void) {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~(0x3 << (9 << 1));
	GPIOA->MODER |= (0x2 << (9 << 1));
	GPIOA->AFR[1] &= ~(0x3 << ((9 - 8) << 2));
	GPIOA->AFR[1] |= (0x1 << ((9 - 8) << 2));
}
/*================================================================================================*/
static void initializeUSART(void) {
	LOG_USART_RCC_ENABLE();
	LOG_USART->BRR = getCoreHz() / LOG_USART_BAUD_DIV / LOG_USART_BAUDRATE;
	LOG_USART->CR1 = USART_CR1_UE | USART_CR1_TE;
}
/*================================================================================================*/
static void printInitialText(void) {
	print(DEFAULT_COLOR);
	print(CLEAR_SCREEN);
	print(RETURN_HOME);
}
/*================================================================================================*/
void note(const char* text, ...) {
	char buffer[70];
	va_list args;

	print(DEFAULT_COLOR);

	va_start(args, text);
	vsprintf(buffer, text, args);
	va_end(args);

	print(buffer);
	print("\n\r");
}
/*================================================================================================*/
void info(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("INFO", DEFAULT_COLOR, text, args);
	va_end(args);
}
/*================================================================================================*/
void warning(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("WARNING", YELLOW_COLOR, text, args);
	va_end(args);
}
/*================================================================================================*/
void error(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("ERROR", RED_COLOR, text, args);
	va_end(args);
}
/*================================================================================================*/
void critical(const char* text, ...) {
	va_list args;
	va_start(args, text);
	printMessage("CRITICAL", CRITICAL_COLOR, text, args);
	va_end(args);
}
/*================================================================================================*/
void printlnHex(const char* label, unsigned int value) {
	char buffer[30];
	sprintf(buffer, "\t%6.6s = 0x%.8X\n\r", label, value);
	print(buffer);
}
/*================================================================================================*/
/*================================================================================================*/
static void printMessage(const char *label, const char* ctrlSeq, const char* text, va_list args) {
	char buffer[70];
	print(ctrlSeq);
	sprintf(buffer, "[%8.8s] - %10u ms - ", label, getTimeMs());
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
