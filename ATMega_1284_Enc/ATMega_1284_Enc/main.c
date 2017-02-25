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
#include "AES_lib/AESlib.h"


/*** FUNCTION DECLARATIONS ***/

// Standard Starting Code
void disableWDT(void);

// AES
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void hshCBC(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size, uint8_t* hash);


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
	
	aes256_enc_single(key, ciphertext);
	
	// Prints ciphertext
	fprintf(stdout, "\nCiphertext: ");
	
	for(int i = 0; i < 16; i++) {
		fprintf(stdout, "%d ", ciphertext[i]);
	}
	
	aes256_dec_single(key, ciphertext);
	
	// Prints ciphertext
	fprintf(stdout, "\nCiphertext: ");
	
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

// Block Cipher CFB Mode Encryption
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size) {
	// Create address counter and block buffer
	uint16_t _address = 0;
	uint8_t  _buffer[16];


	
	// Copy IV to buffer for First Round
	for(uint8_t i = 0; i < 16; i++) {
		_buffer[i] = IV[i];
	}	
	
	// Encryption Rounds
	while(_address <= size) {
		
		// Encrypt buffer in place
		aes256_enc_single(key, _buffer);
		
		// XOR plaintext with encrypted buffer. Store in place.
		for(uint8_t i = 0; i < 16; i++) {
			data[_address + i] ^= _buffer[i];
		}
		
		// Copy next block to buffer
		for(uint8_t i = 0; i < 16; i++) {
			_buffer[i] = data[_address + i];
		}
		
		// Move address to next block
		_address += 16;
	}
}

// Block Cipher CFB Mode Decryption
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size) {
	// Create address counter and block buffer
	uint16_t _address = 0;
	uint8_t  _buffer1[16];
	uint8_t  _buffer2[16];
	
	
	
	// Copy IV to buffer1 for First Round
	for(uint8_t i = 0; i < 16; i++) {
		_buffer1[i] = IV[i];
	}
	
	// Encryption Rounds
	while(_address <= size) {
		
		// Encrypt buffer1 in place
		aes256_enc_single(key, _buffer1);
		
		// Copy ciphertext to buffer2
		for(uint8_t i = 0; i < 16; i++) {
			_buffer2[i] = data[_address + i];
		}
		
		// XOR buffer1 with ciphertext. Store in data
		_data[_address + i]
		
		
		
	}
}

// Block Cipher CBC Mode Hash
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size, uint8_t* hash) {
	
}