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

// For AES lib
//#include "AES_lib/aes256_enc.h"


/*** FUNCTION DECLARATIONS ***/

void disableWDT(void);



/*** VARIABLES & DEFINITIONS ***/

// UART Setup
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
char rec[50];

// AES Setup
uint8_t plaintext[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t ciphertext[16];
uint8_t key[32]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t j             = 0;

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
	fprintf(stdout, "Hello, world! You ready for some AES Encryption? \n\n");
	
	// Prints plaintext
	fprintf(stdout, "Plaintext: ");
	
	for(int i = 0; i < 16; i++) {
		fprintf(stdout, "%d ", plaintext[i]);
	}
	
	// Prints key
	fprintf(stdout, "\nKey: ");
	
	for(int i = 0; i < 32; i++) {
		fprintf(stdout, "%d ", key[i]);
	}
	
	// Prints random value
	fprintf(stdout, "\nJ: %d", j);
	
	// copies plaintext into buffer
	for(int i = 0; i < 16; i++) {
		ciphertext[i] = plaintext[i];
	}
	
	//aes_cenc(ciphertext, key, &j);
	
	// Prints ciphertext
	fprintf(stdout, "Ciphertext: ");
	
	for(int i = 0; i < 16; i++) {
		fprintf(stdout, "%d ", ciphertext[i]);
	}

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