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


/*** FUNCTION DECLARATIONS ***/



/*** VARIABLES & DEFINITIONS ***/

#define LED PINB0


/*** Code ***/

int main(void) {
	/* Setup & Initialization */
	
	DDRB |= (1<<LED);

    while (1) {
		/* Loop */
		
		PORTB ^= (1<<LED);
		_delay_ms(500);
		
    }
}



/*** Interrupt Service Routines ***/



/*** Function Bodies ***/