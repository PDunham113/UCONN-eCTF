/*
 * ATMega1284_VarClk.c
 *
 * Created: 2/12/2017 3:09:57 PM
 * Author : patri
 */ 

#define F_CPU_8 8000000UL
#define F_CPU_1 1000000IL
#define BAUD  115200UL
#define RAND_MAX 255

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

// UART Read Buffer
char rec[50];

// TIMER0 and TIMER2 Control Variables
unsigned long millis = 0;
uint8_t fastClk      = 1;

// Blink Control Variables
#define LED		 PINB0
#define INTERVAL 1000

unsigned long prevMillis = 0;


/*
 * Send character c down the UART Tx, wait until tx holding register
 * is empty. (From ECE 3411 UART Library)
 */
int uart_putchar(char c, FILE *stream) {

	if (c == '\a')
	{
		fputs("*ring*\n", stderr);
		return 0;
	}

	if (c == '\n')
	uart_putchar('\r', stream);
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;

	return 0;
}

/*
 * Receive a character from the UART Rx. (From ECE 3411 UART Library)
 *
 * This features a simple line-editor that allows to delete and
 * re-edit the characters entered, until either CR or NL is entered.
 * Printable characters entered will be echoed using uart_putchar().
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
 * Successive calls to uart_getchar() will be satisfied from the
 * internal buffer until that buffer is emptied again.
 */
int uart_getchar(FILE *stream)
{
  uint8_t c;
  char *cp, *cp2;
  static char b[80];
  static char *rxp;

  if (rxp == 0)
    for (cp = b;;)
      {
	loop_until_bit_is_set(UCSR0A, RXC0);
	if (UCSR0A & _BV(FE0))
	  return _FDEV_EOF;
	if (UCSR0A & _BV(DOR0))
	  return _FDEV_ERR;
	c = UDR0;
	/* behaviour similar to Unix stty ICRNL */
	if (c == '\r')
	  c = '\n';
	if (c == '\n')
	  {
	    *cp = c;
	    uart_putchar(c, stream);
	    rxp = b;
	    break;
	  }
	else if (c == '\t')
	  c = ' ';

	if ((c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') ||
	    c >= (uint8_t)'\xa0')
	  {
	    if (cp == b + 80 - 1)
	      uart_putchar('\a', stream);
	    else
	      {
		*cp++ = c;
		uart_putchar(c, stream);
	      }
	    continue;
	  }

	switch (c)
	  {
	  case 'c' & 0x1f:
	    return -1;

	  case '\b':
	  case '\x7f':
	    if (cp > b)
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;

	  case 'r' & 0x1f:
	    uart_putchar('\r', stream);
	    for (cp2 = b; cp2 < cp; cp2++)
	      uart_putchar(*cp2, stream);
	    break;

	  case 'u' & 0x1f:
	    while (cp > b)
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;

	  case 'w' & 0x1f:
	    while (cp > b && cp[-1] != ' ')
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;
	  }
      }

  c = *rxp++;
  if (c == '\n')
    rxp = 0;

  return c;
}

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// Initializes UART0 as a standard serial port
void initUART0(unsigned long _freq, unsigned long _baud) {
	UCSR0A |= (1<<U2X0);
	UBRR0L = (_freq / (8UL * _baud)) - 1;
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

// Initializes UART1 as a standard serial port
void initUART1(unsigned long _freq, unsigned long _baud) {
	UCSR1A |= (1<<U2X1);
	UBRR1L = (_freq / (8UL * _baud)) - 1;
	UCSR1B |= (1<<RXEN1)|(1<<TXEN1);
}

// Initializes Timer 0 as a 1ms Timer
void initTimer0(void) {
	TIMSK0 |= (1<<OCIE0A);  // Enables Interrupt
	TCCR0A |= (1<<WGM01);   // CTC Mode
	TCCR0B |= (1<<CS01)|(1<<CS00);    // /64 prescaler
	OCR0A   = 124;          // 125 counts @ 8Mhz == 1 ms overflow
}

// Initializes Timer 2 as clock-switching timer
void initTimer2(void) {
	TIMSK0 |= (1<<OCIE0A);  // Enables Interrupt
	TCCR0A |= (1<<WGM01);   // CTC Mode
	TCCR0B |= (1<<CS02)|(1<<CS00);    // /1024 prescaler
	OCR2A   = rand();          // Pseudorandom overflow value 
}

// Sets UART0, UART1, and TIMER0 for 8Mhz operation
// Assumes TIMER0 is 1ms timer
// Assumes UART0, UART1 operating at BAUD
void setFor8MHz(void) {
	// Removes clock divisor
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	
	// Updates Baud Rate Generators
	UBRR0L = (F_CPU_8 / (8UL * BAUD)) - 1;
	UBRR1L = (F_CPU_8 / (8UL * BAUD)) - 1;
	
	// Updates Timer 0
	TCCR0B |= (1<<CS00);
}

// Sets UART0, UART1, and TIMER0 for 1Mhz operation
// Assumes TIMER0 is 1ms timer
// Assumes UART0, UART1 operating at BAUD
void setFor1MHz(void) {
		// Sets /8 clock divisor
		CLKPR = (1<<CLKPCE);
		CLKPR = (1<<CLKPS1)|(1<<CLKPS0);
	
	// Updates Baud Rate Generators
	UBRR0L = (F_CPU_1 / (8UL * BAUD)) - 1;
	UBRR1L = (F_CPU_1 / (8UL * BAUD)) - 1;
	
	// Updates Timer 0
	TCCR0B &= ~(1<<CS00);
}

ISR(TIMER0_COMPA_vect) {
	millis++;
}

ISR(TIMER2_COMPA_vect) {
	OCR2A = rand();
	
	if(fastClk) {
		fastClk = 0;
		setFor1MHz();
		
	}
	else {
		fastClk = 1;
		setFor8MHz();
	}
}



int main(void)
{
	// Only UART0 is configured to print text
	initUART0(F_CPU_8, BAUD);
	initUART1(F_CPU_8, BAUD);
	
	stdout = stdin = stderr = &uart_str;
	
	fprintf(stdout, "Hello, world! =)\n\n");
    /* Replace with your application code */
    while (1) 
    {
		if((millis - prevMillis) >= INTERVAL) {
			prevMillis = millis;
			PORTB ^= (1<<LED);
		}
			
    }
}

