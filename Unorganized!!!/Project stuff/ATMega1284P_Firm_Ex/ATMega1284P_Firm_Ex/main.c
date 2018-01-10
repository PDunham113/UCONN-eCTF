/*
 ATMega1284P_Firm_Ex.c
 *
 * Created: $time$
 * Author : Patrick Dunham
 *
 * Info   : Simple 1 Hz Blink program. Intended to be uploaded through a bootloader. Will be made more complex to test bootloader limits.
 *
 * External Hardware:
 *		- 20MHz Crystal
 *		- LED on PINB0
 *		- LED on PINB1
 *		- UART <-> USB cable on UART0
 *		- UART <-> USB cable on UART1
 *
 */

#define F_CPU 20000000

/*** INCLUDES ***/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
//#include "uart.h"


/*** FUNCTION DECLARATIONS ***/

void disableWDT(void);


/*** VARIABLES & DEFINITIONS ***/

#define LED_PIN PINB0
#define LED_PERIOD 1000

/*** Code ***/

int main(void) {
	/* Setup & Initialization */
	
	// Cleans up from bootloader exit.
	cli();
	disableWDT();
	
	// Sets LED Pin as an output.
	DDRB |= (1<<LED_PIN);

    while (1) {
		/* Loop */
		
		// Toggles LED state when needed
		PORTB ^= (1<<LED_PIN);
		_delay_ms(LED_PERIOD / 2);
		
    }
}


/*** Interrupt Service Routines ***/



/*** Function Bodies ***/

// Disables Watchdog Timer, ensuring firmware runs properly.
void disableWDT(void) {
	// Make sure we reset the timer. Don't want to get caught in a loop! 
	wdt_reset();
	
	// First, we clear the WDT Reset flag. This prevents any overwriting of our WDT settings.
	MCUSR &= ~(1<<WDRF);
	
	// Then, we clear the WDT Control register. Now it's off for good.
	WDTCSR |= (1<<WDCE)|(1<<WDE);
	WDTCSR = 0x00;
}