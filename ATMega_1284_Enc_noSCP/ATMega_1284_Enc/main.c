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
#include "AES_lib/AESlib.h"


/*** FUNCTION DECLARATIONS ***/

// Standard Starting Code
void disableWDT(void);

// AES
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* hash, uint16_t size);


/*** VARIABLES & DEFINITIONS ***/

// UART Setup
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
char rec[50];

// AES Setup
#define MESSAGE_LENGTH 32

uint8_t plaintext[MESSAGE_LENGTH]  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t ciphertext[MESSAGE_LENGTH];
uint8_t key[32]		   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t IV[16]         = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
uint8_t hash[16]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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
	
	
	// Copies plaintext into buffer
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		ciphertext[i] = plaintext[i];
	}
	
	encCFB(key, ciphertext, IV, MESSAGE_LENGTH);
	
	// Prints ciphertext
	fprintf(stdout, "\nCiphertext:\t\t");
	
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%X ", ciphertext[i]);
	}
	
	decCFB(key, ciphertext, IV, MESSAGE_LENGTH);
	
	// Prints decrypted ciphertext
	fprintf(stdout, "\nDecrypted Ciphertext:\t");
	
	for(int i = 0; i < MESSAGE_LENGTH; i++) {
		fprintf(stdout, "%d ", ciphertext[i]);
	}
	
	// Calculates hash
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
 * Disables Watchdog Timer, ensuring firmware runs properly.
 * <p>
 * This method should always be called upon firmware launch, following @code cli();.
 * First, it resets the Watchdog Timer. Next, it resets the Watchdog Timer Reset Flag
 * in the @code MCUSR register. Finally, it turns off the Watchdog Timer completely.
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



/** 
 * Encrypts data in-place using AES-256 in CFB Mode
 * <p>
 * This method uses a standard AES-256 encryption kernel to encrypt data in CFB
 * Mode. This essentially turns the block cipher into a self-correcting stream
 * cipher. In addition, the corresponding decryption method can also use the 
 * AES-256 encryption kernel, saving code space. The code will be encrypted block
 * by block and stored in the same memory space used for the plaintext. This
 * results in drastic memory savings. The data array byte count MUST be divisible
 * by 16. This can be achieved through padding.
 * <p>
 * @param key Pointer to 32-byte array containing the AES-256 key.
 * @param data Pointer to data array. Begins as plaintext, ends as ciphertext.
 * @param IV Pointer to a 16-byte random Initialization Vector
 * @param size Size in bytes of data array. Must be divisible by 16.
 */
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size) {
	// Create address counter and block buffer
	uint16_t _address = 0;
	uint8_t  _buffer[16];


	
	// Copy IV to buffer for First Round
	for(uint8_t i = 0; i < 16; i++) {
		_buffer[i] = IV[i];
	}	
	
	// Encryption Rounds
	while(_address < size) {
		
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



/** 
 * Decrypts data in-place using AES-256 in CFB Mode
 * <p>
 * This method uses a standard AES-256 encryption kernel to decrypt data in CFB
 * Mode. This essentially turns the block cipher into a self-correcting stream
 * cipher. Unlike other block cipher modes, the decryption method can also use the
 * AES-256 encryption kernel, saving code space. The code will be decrypted block
 * by block and stored in the same memory space used for the ciphertext. This
 * results in drastic memory savings. The data array byte count MUST be divisible
 * by 16. This should have been accounted for in the encryption function.
 * <p>
 * @param key Pointer to 32-byte array containing the AES-256 key.
 * @param data Pointer to data array. Begins as ciphertext, ends as plaintext.
 * @param IV Pointer to a 16-byte random Initialization Vector
 * @param size Size in bytes of data array. Must be divisible by 16.
 */
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
	while(_address < size) {
		
		// Encrypt buffer1 in place
		aes256_enc_single(key, _buffer1);
		
		// Copy ciphertext to buffer2
		for(uint8_t i = 0; i < 16; i++) {
			_buffer2[i] = data[_address + i];
		}
		
		// XOR buffer1 with ciphertext. Store in data
		for(uint8_t i = 0; i < 16; i++) {
			data[_address + i] = _buffer1[i] ^ _buffer2[i];
		}
		
		// Copy buffer2 to buffer1
		for(uint8_t i = 0; i < 16; i++) {
			_buffer1[i] = _buffer2[i];
		}
		
		_address += 16;
	}
}



/** 
 * Generates a 128-bit hash using AES-256 CBC-MAC
 * <p>
 * This method uses a standard AES-256 encryption kernel to generate a 128-byte
 * Message Authentication Code (MAC) by running the cipher in CBC Mode. Only the
 * last encrypted block is saved. This hash must be used for messages with fixed
 * sizes only, and the encryption key must be different than the one used for the
 * message encryption. The data is left untouched, unlike with @code encCFB() and
 * @code decCFB(). The hash parameter must be initialized to zero before calling
 * this function.
 * <p>
 * @param key Pointer to 32-byte array containing the AES-256 key.
 * @param data Pointer to data array. Begins as ciphertext, ends as plaintext.
 * @param hash Pointer to a 16-byte hash array. Must be initialized to all zeros.
 * @param size Size in bytes of data array. Must be divisible by 16.
 */
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* hash, uint16_t size) {
	uint16_t _address = 0;
	
	// Hashing Rounds
	while(_address < size) {
		// XOR current hash with plaintext
		for(uint8_t i = 0; i < 16; i++) {
			hash[i] ^= data[i];
		}
		
		// Encrypt current hash in place
		aes256_enc_single(key, hash);
		
		// Increment address
		_address += 16;


	}
	
}


/** 
 * Decodes AES-256 key from key-inverse pair. 
 * <p>
 * This method decodes a standard AES-256 key from its side-channel resistant 
 * key-inverse pairing. With the key-inverse pair, a logical 1 is encoded as '0b10',
 * and a logical 0 is encoded as '0b01'. This encoding means that each byte stored in
 * flash contains an even number of 1's and 0's, making power side-channel attacks
 * versus key reading significantly more difficult. This method provides a means to 
 * decode the key-inverse pair into the original key.
 * <p>
 * @param encodedKey Pointer to 64-byte array containing the key-inverse pair
 * @param key Pointer to the 32-byte array containing the to-be decoded AES-256 key
 */
void decodeAESKey(uint8_t* encodedKey, uint8_t key) {

}