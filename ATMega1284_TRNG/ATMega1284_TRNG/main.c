/*
 * ATMega1284 TRNG.c
 *
 * Created: 2/10/2017 3:05:27 PM
 * Author : patri
 *
 * Info: Proof that the ATMega1284 watchdog timer jitter
 *		 can be used to generate quality random numbers
 *       for cryptographic use.
 */ 

/*** INCLUDES ***/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "uart.h"



/*** FUNCTION DECLARATIONS ***/



/*** VARIABLES & DEFINITIONS ***/

// UART Read Buffer & File Setup
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
char rec[50];

// TRNG Buffer & Control Variables
uint8_t entropyBuffer[32];
uint8_t entropyBufferIndex = 0;

// Random Number
uint32_t randomNumber = 0;



/*** CODE ***/

int main(void) {
	/* Setup & Initialization */
	
	// Kick watchdog
	wdt_reset();
	
	// Disable interrupts and watchdog on reset
	cli();
	WDTCSR |= (1<<WDCE)|(1<<WDE);
	WDTCSR = 0x00;
	
	// Setup watchdog timer to reset every 16ms and trigger interrupt
	WDTCSR |= (1<<WDCE)|(1<<WDE);
	WDTCSR = (1<<WDIE);
	
	// Setup Timer 0
	TCCR0B |= (1<<CS00);
	
	// Initialize UART
	uart_init();
	stdout = stdin = stderr = &uart_str;
	
	// Tell the world we're ready
	fprintf(stdout, "Hello, world! Printing random numbers..\n\n");
	
	// Enable global interrupts
	sei();	
	
	
	
	/* Loop */
	
    while (1) {
		
    }
}



/*** INTERRUPT SERVICE ROUTINES ***/

ISR(WDT_vect) {
	// 32-byte buffer used to hold clock bytes
	entropyBuffer[entropyBufferIndex] = TCNT0;
	entropyBufferIndex++;
	
	// When full, bytes fed into implementation of Jenkins'
	// one-at-a-time hash function. (recommended by Entropy
	// library)
	if(entropyBufferIndex >= 32) {
		randomNumber = 0;
		
		for(int i = 0; i < 32; i++) {
			randomNumber += entropyBuffer[i];
			randomNumber += (randomNumber << 10);
			randomNumber ^= (randomNumber >> 6);
		}
		
		randomNumber += (randomNumber << 3);
		randomNumber ^= (randomNumber >> 11);
		randomNumber += (randomNumber << 15);
		
		fprintf(stdout, "%X%X\n", (unsigned int)(randomNumber>>16), (unsigned int)randomNumber);
		
		entropyBufferIndex = 0;
	}
}

