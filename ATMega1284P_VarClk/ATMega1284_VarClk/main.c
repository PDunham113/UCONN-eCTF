/*
 * ATMega1284_VarClk.c
 *
 * Created: 2/12/2017 3:09:57 PM
 * Author : Patrick Dunham
 *
 * Info   : Demo code for ATMega variable clock speed. Currently switches between 1MHz and 8MHz at a set 4s interval for visual demonstration. Actual implementation
 *			should use a separate timer (TIMER2?) to switch clock at random intervals with greater frequency during decryption. Unfortunately, during UART transmission,
 *			the ATMega must be in 8MHz mode for proper baud rate generation.
 *
 * External Hardware: 
 *		- LED on PINB0
 *		- LED on PINB1
 *		- UART <-> USB cable on UART0
 *		- UART <-> USB cable on UART1
 *
 */ 

/*** INCLUDES ***/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>



/*** FUNCTION DECLARATIONS ***/

// UART Init, Read, Write
void initUART0(void);
void initUART1(void);

int UART0WriteChar(char, FILE*);
int UART1WriteChar(char, FILE*);

int UART0ReadChar(FILE*);
int UART1ReadChar(FILE*);

// TIMER Init
void initTimer0(void);
void initTimer1(void);

// Clock Control
void setFor1MHz(void);
void setFor8MHz(void);



/*** VARIABLES & DEFINITIONS ***/

// UART Read Buffer & File setup
FILE UART0_str = FDEV_SETUP_STREAM(UART0WriteChar, UART0ReadChar, _FDEV_SETUP_RW);
FILE UART1_str = FDEV_SETUP_STREAM(UART1WriteChar, UART1ReadChar, _FDEV_SETUP_RW);

char rec[50];

// TIMER0 Control Variables
volatile unsigned long millis = 0;

// Clock Variation Control Variables
#define INTERVAL_CLK 4000

uint8_t fastClk				= 1;
unsigned long prevMillisCLK = 0;

// Blink Control Variables
#define LED_YELLOW	 PINB0
#define LED_RED	     PINB1
#define INTERVAL_LED 100

unsigned long prevMillisLED = 0;



/*** Code ***/

int main(void) {
	/* Setup & Initialization */
	
	// Initialize UARTs
	initUART0();
	initUART1();
	
	// Initialize Timers
	initTimer0();
	initTimer1();
	
	// Initialize Output Pins
	DDRB |= (1<<LED_RED)|(1<<LED_YELLOW);
	
	// Tell the world we're ready
	fprintf(&UART0_str, "Hello, world! =) This is UART0 speaking!!!\n\n");
	fprintf(&UART1_str, "Hello, world! =D This is UART1 speaking!!!\n\n");
	
	// Enable global interrupts
	sei();
	
	
	
	/* Loop */
	
    while (1) {
		// Toggle the yellow LED every interval
		if((millis - prevMillisLED) >= INTERVAL_LED) {
			prevMillisLED = millis;
			
			PORTB ^= (1<<LED_YELLOW);
		}
		
		// Switch the clock rate every interval
		// Can and should be implemented in an interrupt in the bootloader. This format allows
		// for longer, human-noticeable delays
		if((millis - prevMillisCLK) >= INTERVAL_CLK) {
			prevMillisCLK = millis;
			
			if(fastClk) {
				fastClk = 0;
				setFor1MHz();
			}
			else {
				fastClk = 1;
				setFor8MHz();
			}
		}
			
    }
}



/*** Interrupt Service Routines ***/

// Increments 1ms timer
ISR(TIMER0_COMPA_vect) {
	millis++;
}

// Toggles red LED every 800,000 cycles (100ms in 8MHz mode, 800ms in 1MHz mode)
// Visual demonstration of changing clock speed, not required for operation
ISR(TIMER1_COMPA_vect) {
	PORTB ^= (1<<LED_RED);
}



/*** Function Bodies ***/

// Waits for UART0 TX buffer to empty, sends character
// Line editor:
//		- replaces '\n' with "\r\n"
//		- replaces '\a' with "*ring*\n*" on stderr
// (From ECE 3411 UART Library)
int UART0WriteChar(char c, FILE *stream) {
	// Line Editor -- implements ASCII alarm control character
	if (c == '\a') {
		fputs("*ring*\n", stderr);
		return 0;
	}

	// Line Editor -- replaces with '\n' with "\r\n" so Windows is happy
	if (c == '\n') {
		UART0WriteChar('\r', stream);
	}
	
	// Waits for last character to send, loads new character
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;

	return 0;
}

// Waits for UART1 TX buffer to empty, sends character
// Line editor:
//		- replaces '\n' with "\r\n"
//		- replaces '\a' with "*ring*\n*" on stderr
// (From ECE 3411 UART Library)
int UART1WriteChar(char c, FILE *stream) {
	// Line Editor -- implements ASCII alarm control character
	if (c == '\a') {
		fputs("*ring*\n", stderr);
		return 0;
	}

	// Line Editor -- replaces with '\n' with "\r\n" so Windows is happy
	if (c == '\n') {
		UART1WriteChar('\r', stream);
	}
	
	// Waits for last character to send, loads new character
	loop_until_bit_is_set(UCSR1A, UDRE1);
	UDR1 = c;

	return 0;
}

// Has been reformatted. Description pending reformat. HAS NOT BEEN TESTED
/*
 * Receive a character from the UART Rx. (From ECE 3411 UART Library)
 *
 * This features a simple line-editor that allows to delete and
 * re-edit the characters entered, until either CR or NL is entered.
 * Printable characters entered will be echoed using UART0WriteChar().
 *
 * Editing characters:
 *
 * . \b (BS) or \177 (DEL) delete the previous character
 * . ^u kills the entire input buffer
 * . ^w deletes the previous word
 * . ^r sends a CR, and then reprints the buffer
 * . \t will be replaced by a single space
 *
 * All other control characters will be ignored.
 *
 * The internal line buffer is RX_BUFSIZE (80) characters long, which
 * includes the terminating \n (but no terminating \0).  If the buffer
 * is full (i. e., at RX_BUFSIZE-1 characters in order to keep space for
 * the trailing \n), any further input attempts will send a \a to
 * uart_putchar() (BEL character), although line editing is still
 * allowed.
 *
 * Input errors while talking to the UART will cause an immediate
 * return of -1 (error indication).  Notably, this will be caused by a
 * framing error (e. g. serial line "break" condition), by an input
 * overrun, and by a parity error (if parity was enabled and automatic
 * parity recognition is supported by hardware).
 *
 * Successive calls to UART0ReadChar() will be satisfied from the
 * internal buffer until that buffer is emptied again.
 */
int UART0ReadChar(FILE *stream)
{
	uint8_t c = 0;
	char *cp, *cp2;
	static char b[80];
	static char *rxp;

	if (rxp == 0) {
		for (cp = b;;) {
			loop_until_bit_is_set(UCSR0A, RXC0);
		
			if (UCSR0A & _BV(FE0)) {
				return _FDEV_EOF;
			}
		
			if (UCSR0A & _BV(DOR0)) {
				return _FDEV_ERR;
			}
		
			c = UDR0;
	
			/* behavior similar to Unix STTY ICRNL */
			if (c == '\r') {
				c = '\n';
			}
			
			if (c == '\n') {
				*cp = c;
				UART0WriteChar(c, stream);
				rxp = b;
				break;
			}
			else if (c == '\t') {
				c = ' ';
			}
	
			if ((c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') || c >= (uint8_t)'\xa0') {
				if (cp == b + 80 - 1) {
					UART0WriteChar('\a', stream);
				}
				else {
					*cp++ = c;
					UART0WriteChar(c, stream);
				}
			
				continue;
			}

			switch (c) {
				case 'c' & 0x1f:
					return -1;

				case '\b':
				case '\x7f':
					if (cp > b) {
						UART0WriteChar('\b', stream);
						UART0WriteChar(' ', stream);
						UART0WriteChar('\b', stream);
						cp--;
					}
					break;

				case 'r' & 0x1f:
					UART0WriteChar('\r', stream);
					for (cp2 = b; cp2 < cp; cp2++) {
						UART0WriteChar(*cp2, stream);
					}
					break;

				case 'u' & 0x1f:
					while (cp > b) {
						UART0WriteChar('\b', stream);
						UART0WriteChar(' ', stream);
						UART0WriteChar('\b', stream);
						cp--;
					}
					break;

				case 'w' & 0x1f:
					while (cp > b && cp[-1] != ' ') {
						UART0WriteChar('\b', stream);
						UART0WriteChar(' ', stream);
						UART0WriteChar('\b', stream);
						cp--;
					}
					break;
			}
		}

		c = *rxp++;
		if (c == '\n') {
			rxp = 0;
		}
	}

	return c;
}

// Has been reformatted. Description pending reformat. HAS NOT BEEN TESTED
/*
 * Receive a character from the UART Rx. (From ECE 3411 UART Library)
 *
 * This features a simple line-editor that allows to delete and
 * re-edit the characters entered, until either CR or NL is entered.
 * Printable characters entered will be echoed using UART0WriteChar().
 *
 * Editing characters:
 *
 * . \b (BS) or \177 (DEL) delete the previous character
 * . ^u kills the entire input buffer
 * . ^w deletes the previous word
 * . ^r sends a CR, and then reprints the buffer
 * . \t will be replaced by a single space
 *
 * All other control characters will be ignored.
 *
 * The internal line buffer is RX_BUFSIZE (80) characters long, which
 * includes the terminating \n (but no terminating \0).  If the buffer
 * is full (i. e., at RX_BUFSIZE-1 characters in order to keep space for
 * the trailing \n), any further input attempts will send a \a to
 * uart_putchar() (BEL character), although line editing is still
 * allowed.
 *
 * Input errors while talking to the UART will cause an immediate
 * return of -1 (error indication).  Notably, this will be caused by a
 * framing error (e. g. serial line "break" condition), by an input
 * overrun, and by a parity error (if parity was enabled and automatic
 * parity recognition is supported by hardware).
 *
 * Successive calls to UART0ReadChar() will be satisfied from the
 * internal buffer until that buffer is emptied again.
 */
int UART1ReadChar(FILE *stream)
{
	uint8_t c = 0;
	char *cp, *cp2;
	static char b[80];
	static char *rxp;

	if (rxp == 0) {
		for (cp = b;;) {
			loop_until_bit_is_set(UCSR1A, RXC1);
		
			if (UCSR1A & _BV(FE1)) {
				return _FDEV_EOF;
			}
		
			if (UCSR1A & _BV(DOR1)) {
				return _FDEV_ERR;
			}
		
			c = UDR1;
	
			/* behavior similar to Unix STTY ICRNL */
			if (c == '\r') {
				c = '\n';
			}
			
			if (c == '\n') {
				*cp = c;
				UART1WriteChar(c, stream);
				rxp = b;
				break;
			}
			else if (c == '\t') {
				c = ' ';
			}
	
			if ((c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') || c >= (uint8_t)'\xa0') {
				if (cp == b + 80 - 1) {
					UART1WriteChar('\a', stream);
				}
				else {
					*cp++ = c;
					UART1WriteChar(c, stream);
				}
			
				continue;
			}

			switch (c) {
				case 'c' & 0x1f:
					return -1;

				case '\b':
				case '\x7f':
					if (cp > b) {
						UART1WriteChar('\b', stream);
						UART1WriteChar(' ', stream);
						UART1WriteChar('\b', stream);
						cp--;
					}
					break;

				case 'r' & 0x1f:
					UART1WriteChar('\r', stream);
					for (cp2 = b; cp2 < cp; cp2++) {
						UART1WriteChar(*cp2, stream);
					}
					break;

				case 'u' & 0x1f:
					while (cp > b) {
						UART1WriteChar('\b', stream);
						UART1WriteChar(' ', stream);
						UART1WriteChar('\b', stream);
						cp--;
					}
					break;

				case 'w' & 0x1f:
					while (cp > b && cp[-1] != ' ') {
						UART1WriteChar('\b', stream);
						UART1WriteChar(' ', stream);
						UART1WriteChar('\b', stream);
						cp--;
					}
					break;
			}
		}

		c = *rxp++;
		if (c == '\n') {
			rxp = 0;
		}
	}

	return c;
}

// Initializes UART0 as a standard serial port with baud rate of 115200 @ 8MHz clock
void initUART0(void) {
	UCSR0A |= (1<<U2X0);
	UBRR0L = 8;
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

// Initializes UART0 as a standard serial port with baud rate of 115200 @ 8MHz clock
void initUART1(void) {
	UCSR1A |= (1<<U2X1);
	UBRR1L = 8;
	UCSR1B |= (1<<RXEN1)|(1<<TXEN1);
}

// Initializes Timer 0 as a 1ms Timer
void initTimer0(void) {
	TIMSK0 |= (1<<OCIE0A);  // Enables Interrupt
	TCCR0A |= (1<<WGM01);   // CTC Mode
	TCCR0B |= (1<<CS01)|(1<<CS00);    // /64 prescaler
	OCR0A   = 124;          // 125 counts @ 8Mhz == 1 ms overflow
}

// Initializes Timer 1, overflows every 800,000 cycles
void initTimer1(void) {
	TIMSK1 |= (1<<OCIE1A);  // Enables Interrupt
	TCCR1B |= (1<<WGM12)|(1<<CS11)|(1<<CS10); // CTC Mode, /64 prescaler
	
	OCR1A   = 12499;          // 12500 counts @ 8Mhz == 100 ms overflow
}

// Sets TIMER0 for 8Mhz operation
// Assumes TIMER0 is 1ms timer
// UART will work properly (3.5% baud error)
void setFor8MHz(void) {
	// Removes clock divisor
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	
	// Updates Timer 0
	TCCR0B |= (1<<CS00);
}

// Sets TIMER0 for 1Mhz operation
// Assumes TIMER0 is 1ms timer
// UART WILL NOT WORK PROPERLY (8.5% baud error)
void setFor1MHz(void) {
	// Sets /8 clock divisor
	CLKPR = (1<<CLKPCE);
	CLKPR = (1<<CLKPS1)|(1<<CLKPS0);
	
	// Updates Timer 0
	TCCR0B &= ~(1<<CS00);
}

