/*
 * UART driver code.
 */

#include <avr/io.h>
#include "uart.h"

#define  F_CPU 7300000UL
#define  BAUD 115200UL



/* UART1 FUNCTIONS */

/** 
 * \brief Initializes UART1 for BAUD = 115200 at F_CPU = 7.3 MHz
 *
 * This function sets USCR1A, UCSR1B, and UCSR1C to enable full-duplex communication
 * over UART1 with standard 8-N-1 transmission. (8-bit data size) (No parity bits) (1 stop bit) 
 * The UBRR1H and UBRR1L baud rate generation registers are set to generate a baud rate of 
 * 115200 bits/sec at a clock of 7.3 MHz.
 *
 * UART1 is used for all essential bootloader communication.
 *
 */
void UART1_init(void)
{
    UBRR1H = 0; // Set the baud rate
    UBRR1L = 7;

    UCSR1A |= (1 << U2X1);

    UCSR1B = (1 << RXEN1) | (1 << TXEN1); // Enable receive and transmit

    // Use 8-bit character sizes
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}



/** 
 * \brief Prints a character on UART1
 *
 * This function idles the processor until UART1 has sent any data queued. Then, it assigns
 * the requested data character to the queue to be sent.
 *
 * Only one character can be queued at a time.
 *
 * \param data The character being sent over UART1
 */
void UART1_putchar(unsigned char data)
{
    while(!(UCSR1A & (1 << UDRE1)))
    {
        // Wait for the last bit to send.
    }
    UDR1 = data;
}



/** 
 * \brief Checks if there is a character available on UART1
 *
 * This function checks UCSR1A to see if any data has arrived over UART1. While
 * \code UART1_getchar() also checks to see if any data has arrived before returning,
 * it is better practice to use this function first. This way, the processor minimizes
 * time in an idle state.
 *
 *\return Boolean value representing the presence of data. TRUE if data is present.
 */
bool UART1_data_available(void)
{
    return (UCSR1A & (1 << RXC1)) != 0;
}



/** 
 * \brief Retrieves a character on UART1
 *
 * This function waits for data to arrive on UART1, and then returns the ASCII character
 * present. It is best practice to check for data using \code UART1_data_available() before
 * calling this function to prevent wasted processor time.
 *
 * \return Character received over UART1
 */
unsigned char UART1_getchar(void)
{
    while (!UART1_data_available())
    {
        /* Wait for data to be received */
    }
    /* Get and return received data from buffer */
    return UDR1;
}



/** 
 * \brief Flushes UART1 character buffer
 *
 * This function informs AVR-GCC that the variable UDR1 is not being used, clearing anything
 * that may have been stored in it. This functions similarly to fflush() in Standard C
 *
 */
void UART1_flush(void)
{
    // Tell the compiler that this variable is not being used
    unsigned char __attribute__ ((unused)) dummy;  // GCC attributes
    while ( UART1_data_available() ) dummy = UDR1;
}



/** 
 * \brief Prints a string to UART1
 *
 * This function provides a simple way to print null-terminated strings to UART1, eliminating unneeded
 * for() and while() loops for things such as debug statements. It will call \code UART1_putchar() until
 * it finds a null character (\0 or 0x00). This is interpreted as the end of the string.
 *
 */
void UART1_putstring(char* str)
{
    int i = 0;
    while(str[i] != 0){
        UART1_putchar(str[i]);
        i++;
    }
}



/* UART0 FUNCTIONS */

/** 
 * \brief Initializes UART0 for BAUD = 115200 at F_CPU = 7.3 MHz
 *
 * This function sets USCR0A, UCSR0B, and UCSR0C to enable full-duplex communication
 * over UART0 with standard 8-N-1 transmission. (8-bit data size) (No parity bits) (1 stop bit) 
 * The UBRR0H and UBRR0L baud rate generation registers are set to generate a baud rate of 
 * 115200 bits/sec at a clock of 7.3 MHz.
 *
 * UART0 is used for debug purposes only. (Release message printing is considered debug)
 *
 */
void UART0_init(void) {
    UBRR0H = 0; // Set the baud rate
    UBRR0L = 7;

    UCSR0A |= (1 << U2X0);

    UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable receive and transmit

    // Use 8-bit character sizes
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}



/** 
 * \brief Prints a character on UART0
 *
 * This function idles the processor until UART0 has sent any data queued. Then, it assigns
 * the requested data character to the queue to be sent.
 *
 * Only one character can be queued at a time.
 *
 * \param data The character being sent over UART0
 */
void UART0_putchar(unsigned char data)
{
    while(!(UCSR0A & (1 << UDRE0)))
    {
        // Wait for the last bit to send
    }
    UDR0 = data;
}



/** 
 * \brief Checks if there is a character available on UART0
 *
 * This function checks UCSR0A to see if any data has arrived over UART0. While
 * \code UART1_getchar() also checks to see if any data has arrived before returning,
 * it is better practice to use this function first. This way, the processor minimizes
 * time in an idle state.
 *
 *\return Boolean value representing the presence of data. TRUE if data is present.
 */
bool UART0_data_available(void)
{
    return (UCSR0A & (1 << RXC0)) != 0;
}



/** 
 * \brief Retrieves a character on UART1
 *
 * This function waits for data to arrive on UART1, and then returns the ASCII character
 * present. It is best practice to check for data using \code UART1_data_available() before
 * calling this function to prevent wasted processor time.
 *
 * \return Character received over UART1
 */
unsigned char UART0_getchar(void)
{
    while(!UART0_data_available())
    {
        /* Wait for data to be received */
    }
    /* Get and return received data from buffer */
    return UDR0;
}



/** 
 * \brief Flushes UART0 character buffer
 *
 * This function informs AVR-GCC that the variable UDR0 is not being used, clearing anything
 * that may have been stored in it. This functions similarly to fflush() in Standard C
 *
 */
void UART0_flush(void)
{
    // Tell the compiler that this variable is not being used
    unsigned char __attribute__ ((unused)) dummy;  // GCC attributes
    while(UART0_data_available())
    {
        dummy = UDR0;
    }
}



/** 
 * \brief Prints a string to UART0
 *
 * This function provides a simple way to print null-terminated strings to UART0, eliminating unneeded
 * for() and while() loops for things such as debug statements. It will call \code UART0_putchar() until
 * it finds a null character (\0 or 0x00). This is interpreted as the end of the string.
 *
 */
void UART0_putstring(char* str)
{
    int i = 0;
    while(str[i] != 0){
        UART0_putchar(str[i]);
        i++;
    }
}
