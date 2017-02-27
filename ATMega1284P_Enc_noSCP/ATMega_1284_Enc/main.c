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
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "uart.h"

// For AES lib
#include "AES_lib.h"

// For Flash Read/Write
#include "avr/boot.h"
#include "avr/pgmspace.h"


/*** FUNCTION DECLARATIONS ***/

// Standard Starting Code
void disableWDT(void);

// Flash read/write code
void programFlashPage(uint32_t pageAddress, uint8_t *data);

/***  DEFINITIONS ***/

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// AES Setup
#define MESSAGE_LENGTH 1024

uint8_t hash[16]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t IV[16]         = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
uint8_t key[32]		   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t plaintext[MESSAGE_LENGTH];
uint8_t ciphertext[MESSAGE_LENGTH];
uint8_t newPlaintext[MESSAGE_LENGTH];

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
	
	// Generates deterministic plaintext
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		plaintext[i] = (uint8_t)i;
	}
	
	// Prints plaintext
	fprintf(stdout, "\nDeterministic Plaintext:\t");
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", plaintext[i]);
	}
	
	// Stores plaintext in FLASH
	for(int i = 0; i < MESSAGE_LENGTH; i += SPM_PAGESIZE) {
		programFlashPage(0x0000 + i, &plaintext[i]);
	}
	
	// Erases plaintext
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		plaintext[i] = 0;
	}
	
	// Prints plaintext
	fprintf(stdout, "\nErased Plaintext:\t");
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", plaintext[i]);
	}
	
	// Reads page of plaintext from FLASH
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		plaintext[i] = pgm_read_byte(i);
	}
	
	// Prints plaintext
	fprintf(stdout, "\nPlaintext:\t\t");
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
	
	/* ENCRYPTION */
	strtEncCFB(key, plaintext, IV, &ctx, ciphertext);
	for(int i = 16; i < MESSAGE_LENGTH; i += 16) {
		contEncCFB(&ctx, &plaintext[i], &ciphertext[i - 16], &ciphertext[i]);
	}
	
	// Prints ciphertext
	fprintf(stdout, "\nCiphertext:\t\t");
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%X ", ciphertext[i]);
	}
	
	/* DECRYPTION */
	strtDecCFB(key, ciphertext, IV, &ctx, newPlaintext);
	for(int i = 16; i < MESSAGE_LENGTH; i += 16) {
		contDecCFB(&ctx, &ciphertext[i], &ciphertext[i - 16], &newPlaintext[i]);
	}
	
	// Prints decrypted ciphertext
	fprintf(stdout, "\nDecrypted Ciphertext:\t");
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", newPlaintext[i]);
	}
	
	/* HASHING */
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



// Programs a 128-byte flash page
void programFlashPage(uint32_t pageAddress, uint8_t *data) {
	int i = 0;
	uint8_t sreg;

	// Disable interrupts
	sreg = SREG;
	cli();

	boot_page_erase_safe(pageAddress);

	for(i = 0; i < SPM_PAGESIZE; i += 2)
	{
		uint16_t w = data[i];    // Make a word out of two bytes
		w += data[i+1] << 8;
		boot_page_fill_safe(pageAddress+i, w);
	}

	boot_page_write_safe(pageAddress);
	boot_rww_enable_safe(); // We can just enable it after every program too

	//Re-enable interrupts
	SREG = sreg;
}