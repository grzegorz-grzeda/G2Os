/*================================================================================================*/
/*
 * g2OsThread.c
 *
 *  Created on: 19.10.2016
 *      Author: grzegorz
 */

/*================================================================================================*/
#include "g2OsThread.h"
/*================================================================================================*/
/*================================================================================================*/
void G2OS_PendSV_Handler(void) __attribute__ ((naked));
/*================================================================================================*/
typedef struct {
	void (*handler)(void*);
	void *parameters;
	char name[20];
	unsigned int stack[OS_THREAD_STACK];
	unsigned int stackPointer;
} Thread;
/*================================================================================================*/
struct {
	Thread threads[OS_MAX_THREAD_CNT];
	int currentThreadNumber;
	int numberOfThreads;
	int work;
} OS;
/*================================================================================================*/
int registerThread(void* watek, const char* nazwa, void* parametry) {
	if (OS.numberOfThreads == OS_MAX_THREAD_CNT)
		return -1;

	int cnt = OS.numberOfThreads++;
	Thread* w = &(OS.threads[cnt]);

	w->handler = watek;
	strncpy(w->name, nazwa, 20);
	w->name[19] = 0;
	w->parameters = parametry;

	w->stack[OS_THREAD_STACK - 1] = 0x41000000; // PSR
	w->stack[OS_THREAD_STACK - 2] = (unsigned int) watek; // PC
	w->stack[OS_THREAD_STACK - 8] = (unsigned int) parametry; // R0
	w->stack[OS_THREAD_STACK - 9] = 0xFFFFFFF1; // LR z przerwania

	w->stackPointer = (unsigned int) (&(w->stack[OS_THREAD_STACK - 17]));

	return 0;
}
/*================================================================================================*/
void runOS(void) {
	OS.currentThreadNumber = -1;
	OS.work = 1;
	while (1)
		;
}
/*================================================================================================*/
void G2OS_PendSV_Handler(void) {
	__asm("PUSH {LR} \n"
			"PUSH {R4-R7} \n"
			"MOV R4, R8 \n"
			"MOV R5, R9 \n"
			"MOV R6, R10 \n"
			"MOV R7, R11 \n"
			"PUSH {R4-R7}");
	if (OS.work) {
		unsigned int stackPtr;
		if (OS.currentThreadNumber >= 0) {
			__asm volatile("mrs %0, msp\n"
					"isb \n"
					"dsb \n"
					:"=r"(stackPtr));
			OS.threads[OS.currentThreadNumber].stackPointer = stackPtr;
		}
		OS.currentThreadNumber++;
		OS.currentThreadNumber %= OS.numberOfThreads;

		stackPtr = OS.threads[OS.currentThreadNumber].stackPointer;
		__asm volatile("msr msp, %0\n"
				"isb \n"
				"dsb \n"
				::"r"(stackPtr));
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
/*                                              EOF                                               */
/*================================================================================================*/
