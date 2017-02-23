/*
 ATMega1284P_Enc.c
 *
 * Created: 2/23/2017 11:46:33 AM
 * Author : Patrick Dunham
 *
 * Info   : Test platform for encryption and decryption. Uses side-channel-resistant AES in CFB Mode.
 *
 * External Hardware:
 *		- 20MHz Crystal
 *		- LED on PINB0
 *		- LED on PINB1
 *		- UART <-> USB cable on UART0
 *		- UART <-> USB cable on UART1
 *
 */

#define F_CPU 20000000UL

/*** INCLUDES ***/

#include <avr/io.h>
//#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "uart.h"


/*** FUNCTION DECLARATIONS ***/

void disableWDT(void);



/*** VARIABLES & DEFINITIONS ***/

// UART Setup
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
char rec[50];

/*** Code ***/

int main(void) {
	/* Setup & Initialization */
	
	// Cleans up from bootloader exit.
	cli();
	disableWDT();
	
	// Initializes UART0
	uart_init();
	
	// Maps UART0 to stdout, letting us fprintf for funsies.
	stdin = stdout = stderr = &uart_str;
	fprintf(stdout, "Hello, world! You ready for some AES Encryption? (y/n)\n\n");
	
	
	

    while (1) {
		/* Loop */
		
		
		
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