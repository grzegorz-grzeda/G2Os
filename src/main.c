#include "g2systemInit.h"
#include "g2systemLog.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	int i = 0;

	info("Witaj swiecie :)");
	while (1) {
		delayMs(500);
		GPIOC->ODR ^= (3 << 8);
		i++;
		info("Ku-ku po raz %d!", i);
		if (i == 9) {
			info("Wyzwalam zdarzenie PendSV!");
			SCB->ICSR |= (1 << 28);
			__DSB();
		}
	}
}
