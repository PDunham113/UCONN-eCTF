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
#define MESSAGE_LENGTH 40960UL
#define KEY_LENGTH     32
#define BLOCK_LENGTH   16

uint8_t hash[BLOCK_LENGTH]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t IV[BLOCK_LENGTH]         = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
uint8_t key[KEY_LENGTH]		     = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t plaintext[SPM_PAGESIZE];
uint8_t ciphertext[SPM_PAGESIZE];
uint8_t blockBuffer[BLOCK_LENGTH];

aes256_ctx_t ctx;

/*** Code ***/

int main(void) {
	/* SETUP & INITIALIZATION */	
	// Cleans up from bootloader exit.
	cli();
	disableWDT();
	
	// Initializes UART0
	uart_init();
	
	// Maps UART0 to stdout, letting us fprintf for funsies.
	stdin = stdout = stderr = &uart_str;
	fprintf(stdout, "\n\nHello, world! You ready for some AES Encryption? \n\n");
	
	
	
	/* PRINTING PARAMETERS */
	// Prints key
	fprintf(stdout, "\nKey:\t\t\t");
	for(int i = 0; i < KEY_LENGTH; i++) {
		fprintf(stdout, "%d ", key[i]);
	}
	
	// Prints Initialization Vector
	fprintf(stdout, "\nIV:\t\t\t");
	for(int i = 0; i < BLOCK_LENGTH; i++) {
		fprintf(stdout, "%d ", IV[i]);
	}
	
	
	
	/* "RECEIVEING PLAINTEXT" */
	for(uint8_t j = 0; j < (MESSAGE_LENGTH / SPM_PAGESIZE); j++) {
		// Generates "received data"
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			plaintext[i] = i;
		}		
		
		// Stores page
		programFlashPage(MESSAGE_LENGTH * 0UL + (uint32_t)j * SPM_PAGESIZE, plaintext);
			
		// Prints page
		fprintf(stdout, "\n\nPlaintext Page Stored@%X:\n", (uint16_t)(MESSAGE_LENGTH * 0UL + (uint32_t)j * SPM_PAGESIZE));
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			fprintf(stdout, "%X ", plaintext[i]);
		}	
	}
	
	
		
	/* ENCRYPTION */
	for(uint8_t j = 0; j < (MESSAGE_LENGTH / SPM_PAGESIZE); j++) {
		// Reads page of plaintext from flash
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			plaintext[i] = pgm_read_byte_far(MESSAGE_LENGTH * 0UL + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		// Encrypts page
		for(int i = 0; i < SPM_PAGESIZE; i += 16) {
			// Check for first block of message or first block of page. These must be handled differently.
			if((j == 0) && (i == 0)) {
				strtEncCFB(key, &plaintext[0], IV, &ctx, &ciphertext[0]);
			}
			else if(i == 0) {
				contEncCFB(&ctx, &plaintext[i], &blockBuffer[0], &ciphertext[i]);
			}
			else {
				contEncCFB(&ctx, &plaintext[i], &ciphertext[i - 16], &ciphertext[i]);
			}
		}
		
		// Saves data for next page
		for(uint8_t i = 0; i < BLOCK_LENGTH; i++) {
			blockBuffer[i] = ciphertext[SPM_PAGESIZE - BLOCK_LENGTH + i];
		}
		
		// Stores page
		programFlashPage(MESSAGE_LENGTH * 1UL + (uint32_t)j * SPM_PAGESIZE, &ciphertext[0]);
		
		// Prints page
		fprintf(stdout, "\n\nCiphertext Page@%X Stored@%X:\n", (uint16_t)(MESSAGE_LENGTH * 0UL + (uint32_t)j * SPM_PAGESIZE), (uint16_t)(MESSAGE_LENGTH * 1UL + (uint32_t)j * SPM_PAGESIZE));
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			fprintf(stdout, "%X ", ciphertext[i]);
		}
	}
	
	
	
	/* DECRYPTION */
	for(uint8_t j = 0; j < (MESSAGE_LENGTH / SPM_PAGESIZE); j++) {
		// Reads page of ciphertext from flash
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			ciphertext[i] = pgm_read_byte_far(MESSAGE_LENGTH * 1UL + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		// Decrypts page
		for(int i = 0; i < SPM_PAGESIZE; i += 16) {
			// Check for first block of message or first block of page. These must be handled differently.
			if((j == 0) && (i == 0)) {
				strtDecCFB(key, &ciphertext[0], IV, &ctx, &plaintext[0]);
			}
			else if(i == 0) {
				contDecCFB(&ctx, &ciphertext[i], &blockBuffer[0], &plaintext[i]);
			}
			else {
				contDecCFB(&ctx, &ciphertext[i], &ciphertext[i - 16], &plaintext[i]);
			}
		}
		
		// Saves data for next page
		for(uint8_t i = 0; i < BLOCK_LENGTH; i++) {
			blockBuffer[i] = ciphertext[SPM_PAGESIZE - BLOCK_LENGTH + i];
		}
		
		// Stores page
		programFlashPage(MESSAGE_LENGTH * 2UL + (uint32_t)j * SPM_PAGESIZE, &plaintext[0]);
		
		// Prints page
		fprintf(stdout, "\n\nDecrypted Page@%X Stored@%X:\n", (uint16_t)(MESSAGE_LENGTH * 1UL + (uint32_t)j * SPM_PAGESIZE), (uint16_t)(MESSAGE_LENGTH * 2UL + (uint32_t)j * SPM_PAGESIZE));
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			fprintf(stdout, "%X ", plaintext[i]);
		}		
	}
	
	
	
	/* HASHING */
	for(uint8_t j = 0; j < (MESSAGE_LENGTH / SPM_PAGESIZE); j++) {
		// Reads page of ciphertext from flash
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			plaintext[i] = pgm_read_byte_far(MESSAGE_LENGTH * 2UL + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		// Hash it
		hashCBC(key, plaintext, hash, SPM_PAGESIZE);
		
		// Print current status
		fprintf(stdout, "\nHash after page@%X:", (uint16_t)(MESSAGE_LENGTH * 2UL + (uint32_t)j * SPM_PAGESIZE));
		for(int i = 0; i < BLOCK_LENGTH; i++) {
			fprintf(stdout, "%X ", hash[i]);
		}
	}
	

    while (1) {
		/* LOOP */
		
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