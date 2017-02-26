/*
 ATMega1284P_Enc_noSCP.c
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
#include "AES_lib.h"


/*** FUNCTION DECLARATIONS ***/

// Standard Starting Code
void disableWDT(void);


/*** VARIABLES & DEFINITIONS ***/

// UART Setup
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
char rec[50];

// AES Setup
#define MESSAGE_LENGTH 64

uint8_t plaintext[MESSAGE_LENGTH]  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t ciphertext[MESSAGE_LENGTH];
uint8_t newPlaintext[MESSAGE_LENGTH];
uint8_t key[32]		   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t IV[16]         = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
uint8_t hash[16]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
aes256_ctx_t ctx;

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
	fprintf(stdout, "\n\nHello, world! You ready for some AES Encryption? \n\n");
	
	// Prints plaintext
	fprintf(stdout, "Plaintext:\t\t");
	
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", plaintext[i]);
	}
	
	// Prints key
	fprintf(stdout, "\nKey:\t\t\t");
	
	for(int i = 0; i < 32; i++) {
		fprintf(stdout, "%d ", key[i]);
	}
	
	// Prints Initialization Vector
	fprintf(stdout, "\nIV:\t\t\t");
	
	for(int i = 0; i < 16; i++) {
		fprintf(stdout, "%d ", IV[i]);
	}
	
	
	/*// Copies plaintext into buffer
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		ciphertext[i] = plaintext[i];
	}*/
	
	// Encryption
	//encCFB(key, ciphertext, IV, MESSAGE_LENGTH);
	strtEncCFB(key, plaintext, IV, &ctx, ciphertext);
	for(int i = 16; i < MESSAGE_LENGTH; i += 16) {
		contEncCFB(&ctx, &plaintext[i], ciphertext, &ciphertext[i]);
	}
	
	// Prints ciphertext
	fprintf(stdout, "\nCiphertext:\t\t");
	
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%X ", ciphertext[i]);
	}
	
	// Decryption
	//decCFB(key, ciphertext, IV, MESSAGE_LENGTH);
	strtDecCFB(key, ciphertext, IV, &ctx, newPlaintext);
	for(int i = 16; i < MESSAGE_LENGTH; i += 16) {
		contDecCFB(&ctx, &ciphertext[i], ciphertext, &newPlaintext[i]);
	}
	
	// Prints decrypted ciphertext
	fprintf(stdout, "\nDecrypted Ciphertext:\t");
	
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", newPlaintext[i]);
	}
	
	// Hashing
	hashCBC(key, plaintext, hash, MESSAGE_LENGTH);
	
	// Prints hash
	fprintf(stdout, "\nHash:\t\t\t");
	
	for(int i = 0; i < 16; i++) {
		fprintf(stdout, "%X ", hash[i]);
	}
	

    while (1) {
		/* Loop */
		
		
		
    }
}


/*** Interrupt Service Routines ***/



/*** Function Bodies ***/

/**
 * \brief Disables Watchdog Timer, ensuring firmware runs properly.
 * 
 * This method should always be called upon firmware launch, following cli();.
 * First, it resets the Watchdog Timer. Next, it resets the Watchdog Timer Reset Flag
 * in the MCUSR register. Finally, it turns off the Watchdog Timer completely.
 * This allows the code to continue without error.
 */
void disableWDT(void) {
	// Make sure we reset the timer. Don't want to get caught in a loop! 
	wdt_reset();
	
	// First, we clear the WDT Reset flag. This prevents any overwriting of our WDT settings.
	MCUSR &= ~(1<<WDRF);
	
	// Then, we clear the WDT Control register. Now it's off for good.
	WDTCSR |= (1<<WDCE)|(1<<WDE);
	WDTCSR = 0x00;
}