/*
 ATMega1284P_Boot.c
 *
 * Created: 2/21/2017 12:01:23 PM
 * Author : Patrick Dunham
 *
 * Info   : ATMega1284P bootloader. It boots, it loads, it does everything!
 *
 * External Hardware:
 *		- 20MHz Crystal (soon to be removed)
 *		- Jumper to ground on PINB2, PINB3
 *		- LED on PINB0
 *		- LED on PINB1
 *		- UART <-> USB cable on UART0
 *		- UART <-> USB cable on UART1
 *
 */

/*
 * bootloader.c
 *
 * If Port B Pin 2 (PB2 on the protostack board) is pulled to ground the 
 * bootloader will wait for data to appear on UART1 (which will be interpretted
 * as an updated firmware package).
 * 
 * If the PB2 pin is NOT pulled to ground, but 
 * Port B Pin 3 (PB3 on the protostack board) is pulled to ground, then the 
 * bootloader will enter flash memory readback mode. 
 * 
 * If NEITHER of these pins are pulled to ground, then the bootloader will 
 * execute the application from flash.
 *
 * If data is sent on UART for an update, the bootloader will expect that data 
 * to be sent in frames. A frame consists of two sections:
 * 1. Two bytes for the length of the data section
 * 2. A data section of length defined in the length section
 *
 * [ 0x02 ]  [ variable ]
 * ----------------------
 * |  Length |  Data... |
 *
 * Frames are stored in an intermediate buffer until a complete page has been
 * sent, at which point the page is written to flash. See program_flash() for
 * information on the process of programming the flash memory. Note that if no
 * frame is received after 2 seconds, the bootloader will time out and reset.
 *
 */



#define F_CPU 20000000UL
#define BAUD 115200UL

/*** INCLUDES ***/

#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include "uart.h"
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "AES_lib.h"
#include "secret_build_output.txt"



/*** FUNCTION DECLARATIONS ***/

void program_flash(uint32_t page_address, unsigned char *data);
void load_firmware(void);
void boot_firmware(void);
void readback(void);
void configure(void);



/*** VARIABLES & DEFINITIONS ***/

#define LED PINB0
#define UPDATE_PIN PINB2
#define READBACK_PIN PINB3
#define CONFIGURE_PIN PINB4

#define OK    ((unsigned char)0x00)
#define ERROR ((unsigned char)0x01)
#define ACK ((unsigned char)0x06)
#define NACK ((unsigned char)0x15)

#define BLOCK_SIZE 16
#define KEY_SIZE   32

#define MAX_PAGE_NUMBER 126UL

#define APPLICATION_SECTION 0UL * MAX_PAGE_NUMBER * SPM_PAGESIZE
#define MESSAGE_SECTION     1UL * (MAX_PAGE_NUMBER - 6) * SPM_PAGESIZE
#define ENCRYPTED_SECTION	1UL * MAX_PAGE_NUMBER * SPM_PAGESIZE
#define DECRYPTED_SECTION	2UL * MAX_PAGE_NUMBER * SPM_PAGESIZE
#define HASH_SECTION        479UL * SPM_PAGESIZE
#define BOOTLDR_SECTION		0x1E000UL
#define BOOTLDR_SIZE		8192UL
#define EEPROM_SIZE			4096UL

uint16_t fw_version EEMEM;

unsigned char CONFIG_ERROR_FLAG = OK;

uint8_t hashKey[KEY_SIZE]     = H_KEY;
uint8_t firmwareKey[KEY_SIZE] = FW_KEY;
uint8_t readbackKey[KEY_SIZE] = RB_KEY;

uint8_t firmwareIV[BLOCK_SIZE] = FW_IV;
uint8_t readbackIV[BLOCK_SIZE] = RB_IV;


/*** Code ***/

int main(void) {
	/*** SETUP & INITIALIZATION ***/
	
	
	// Init UARTs (virtual com port)
	UART1_init();

	UART0_init();
	
	wdt_reset();
	MCUSR &= ~(1<<WDRF);
	
	wdt_disable();
		
	// Configure Port B Pins 2 and 3 as inputs.
	DDRB &= ~((1 << UPDATE_PIN) | (1 << READBACK_PIN)|(1 << CONFIGURE_PIN));

	// Enable pullups - give port time to settle.
	PORTB |= (1 << UPDATE_PIN) | (1 << READBACK_PIN) | (1 << CONFIGURE_PIN);
	
	

	// If jumper is present on pin 2, load new firmware.
	if(!(PINB & (1 << UPDATE_PIN)))
	{
		UART1_putchar('U');
		load_firmware();
	}
	else if(!(PINB & (1 << READBACK_PIN)))
	{
		UART1_putchar('R');
		readback();
	}
	
	else if(!(PINB & (1 << CONFIGURE_PIN)))
	{
		UART1_putchar('C');
		configure();
	}
	
	else
	{
		UART1_putchar('B');
		boot_firmware();
	}
	
	
} // main



/*** Interrupt Service Routines ***/



/*** Function Bodies ***/



/**
 * \brief Interfaces with host configure tool
 *
 */
void configure()
{
	uint8_t pageBuffer[SPM_PAGESIZE];
	uint8_t hash[BLOCK_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		
	//Start the Watchdog Timer
	wdt_enable(WDTO_2S);
	
	//Wait for ACK
	while(UART1_getchar()!=ACK);
	
	//reset Watchdog Timer
	wdt_reset();
	
	
	//generate hash of bootloader
	for (int j = 0; j < BOOTLDR_SIZE; j+=SPM_PAGESIZE)
	{	
		// Get a page of data
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = pgm_read_byte_far(BOOTLDR_SECTION+i+(uint32_t)j*SPM_PAGESIZE);
		}
	
		// Add to the hash
		hashCBC(hashKey, pageBuffer, hash, SPM_PAGESIZE);
	}
	
	//reset Watchdog Timer
	wdt_reset();
	
	//send bootloader hash over UART1
	for(int i = 0; i < BLOCK_SIZE; i++)
	{
		UART1_putchar(hash[i]);
	}
	
	//reset Watchdog Timer
	wdt_reset();
	
	//Wait for ACK
	while(!UART1_data_available())
	{
		__asm__ __volatile__("");
	}
	
	//reset Watchdog Timer
	wdt_reset();
	
	//pc will ack if hash is correct
	if(UART1_getchar() == ACK)
	{	
		//calculate eeprom hash
		for(int i = 0; i < EEPROM_SIZE; i++)
		{
			// Get a page of data
			for(uint8_t i = 0; i < SPM_PAGESIZE; i++) {
				pageBuffer[i] = eeprom_read_byte(&i);
			}
					
			// Add to the hash
			hashCBC(hashKey, pageBuffer, hash, SPM_PAGESIZE);
		}
		
		//reset Watchdog Timer
		wdt_reset();
			
		//send eeprom hash over UART1
		for(int i = 0; i < BLOCK_SIZE; i++)
		{
			UART1_putchar(hash[i]);
		}
		
		//reset Watchdog Timer
		wdt_reset();
		
		//Wait for ACK
		while(!UART1_data_available())
		{
			__asm__ __volatile__("");
		}
		
		//reset Watchdog Timer
		wdt_reset();
		
		//pc will ack if hash is correct
		if(UART1_getchar() == ACK)
		{
			//send counter here
			while(1){__asm__ __volatile__("");}		//wait for reset
			
		}
		
		else
		{
			CONFIG_ERROR_FLAG = ERROR;
			while(1){__asm__ __volatile__("");}		//wait for reset
		}
	}	

}


/**
 * \brief Interfaces with host readback tool. [INCOMPLETE]
 *
 */
void readback(void)
{
    // Start the Watchdog Timer
    wdt_enable(WDTO_2S);

    // Read in start address (4 bytes).
    uint32_t start_addr = ((uint32_t)UART1_getchar()) << 24;
    start_addr |= ((uint32_t)UART1_getchar()) << 16;
    start_addr |= ((uint32_t)UART1_getchar()) << 8;
    start_addr |= ((uint32_t)UART1_getchar());

    wdt_reset();

    // Read in size (4 bytes).
    uint32_t size = ((uint32_t)UART1_getchar()) << 24;
    size |= ((uint32_t)UART1_getchar()) << 16;
    size |= ((uint32_t)UART1_getchar()) << 8;
    size |= ((uint32_t)UART1_getchar());

    wdt_reset();

    // Read the memory out to UART1.
    for(uint32_t addr = start_addr; addr < start_addr + size; ++addr)
    {
        // Read a byte from flash.
        unsigned char byte = pgm_read_byte_far(addr);
        wdt_reset();

        // Write the byte to UART1.
        UART1_putchar(byte);
        wdt_reset();
    }

    while(1) __asm__ __volatile__(""); // Wait for watchdog timer to reset.
}


/**
 * \brief Loads a new firmware image and release message
 * 
 * This function securely loads a new firmware image onto flash, following the
 * procedure outlined below.
 * 
 * 1 - The encrypted firmware image is loaded into the ENCRYPTED_SECTION of flash.
 * 2 - The CBC-MAC of the encrypted firmware image is computed, and compared to the
 *	   CBC-MAC sent.
 *		IF   CORRECT - The bootloader proceeds with the firmware upload.
 *		IF INCORRECT - The bootloader erases ENCRYPTED_SECTION and terminates.
 * 3 - The firmware image is decrypted and stored in the DECRYPTED_SECTION of flash.
 * 4 - The version number is checked versus the current version, and updated in EEPROM
 *		IF   CORRECT - The bootloader proceeds with the firmware upload.
 *		IF INCORRECT - The bootloader erases ENCRYPTED_SECTION and DECRYPTED_SECTION and
 *					   terminate.
 * 5 - The release message is written to the MESSAGE_SECTION
 * 6 - The firmware is written to the APPLICATION_SECTION
 * 7 - The bootloader erases ENCRYPTED_SECTION and DECRYPTED_SECTION and terminates
 *
 */
void load_firmware(void) {
	uint8_t pageBuffer[SPM_PAGESIZE];
	uint8_t decryptedBuffer[SPM_PAGESIZE];
	uint8_t blockBuffer[BLOCK_SIZE];
	
	uint8_t hash[BLOCK_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t hashFlag = 0;
	
	uint16_t currentVersion = eeprom_read_word(&fw_version);
	uint16_t newVersion     = 0x0001;
	
	aes256_ctx_t ctx;
	
	// Start Watchdog Timer
	wdt_enable(WDTO_2S);
	
	
	
	/* GET UART DATA, CALCULATE HASH */
	for(int j = 0; j < MAX_PAGE_NUMBER; j++) {
		// Wait for data
		while(!UART1_data_available()) {
			__asm__ __volatile__("");
		}
		
		// Reset WDT
		wdt_reset();
		
		// Get a page of data
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = (uint8_t)UART1_getchar();
		}
	
		// Write data to Encrypted Section
		program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
		
		if(j != (MAX_PAGE_NUMBER - 1)) {
			// Add to the hash
			hashCBC(hashKey, pageBuffer, hash, SPM_PAGESIZE);
		}
		
		// Get ready for next page
		UART1_putchar(ACK);
		
		// Reset WDT
		wdt_reset();
	}
	
	
	
	/* CHECK HASH */
	for(int i = 0; i < BLOCK_SIZE; i++) {
		if(hash[i] != pageBuffer[i]) {
			hashFlag = 1;
		}
	}
	
	// If hash is wrong, erase and reset
	if(hashFlag) {
		// Send NACK
		UART1_putchar(NACK);
		
		// DEBUG - Tell us hash failed
		UART0_putstring("Hash failed\n");
		
		// Fill pageBuffer with 0xFF
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = 0xFF;
		}
		
		wdt_reset();
		
		// Write over Encrypted Section
		for(int j = 0; j < MAX_PAGE_NUMBER; j++) {
			program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
			wdt_reset();
		}
		
		// Reset
		while(1) {
			__asm__ __volatile__("");
		}
	}
	
	wdt_reset();
	UART1_putchar(ACK);
	
	// DEBUG - Tell us hash succeeded
	UART0_putstring("Hash succeeded\n");
	
	
	
	/* DECRYPT */
	for(int j = 0; j < MAX_PAGE_NUMBER; j++) {
		// Reads page from flash
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = pgm_read_byte_far(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		wdt_reset();
		
		// Decrypts page
		for(int i = 0; i < SPM_PAGESIZE; i += BLOCK_SIZE) {
			if((j == 0) && (i == 0)) {
				strtDecCFB(firmwareKey, &pageBuffer[0], firmwareIV, &ctx, &decryptedBuffer[0]);
			}
			else if(i == 0) {
				contDecCFB(&ctx, &pageBuffer[i], blockBuffer, &decryptedBuffer[0]);
			}
			else {
				contDecCFB(&ctx, &pageBuffer[i], &pageBuffer[i - BLOCK_SIZE], &decryptedBuffer[i]);
			}
		}
		
		wdt_reset();
		
		// Save data for next page
		for(int i = 0; i < BLOCK_SIZE; i++) {
			blockBuffer[i] = pageBuffer[SPM_PAGESIZE - BLOCK_SIZE + i];
		}
		
		// Writes data to Decrypted Section
		program_flash(DECRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, decryptedBuffer);
		
		wdt_reset();
	}
	
	wdt_reset();
	UART1_putchar(ACK);
	
	// DEBUG - Decrypt successful
	UART0_putstring("Decrypt succeeded\n");
	
	
	/* CHECK VERSION */
	// Read version number from Decrypted Section
	newVersion = pgm_read_word_far(DECRYPTED_SECTION);
	
	// Compare versions
	if((newVersion != 0) && (newVersion <= currentVersion)) {
		// Firmware Too Old
		UART1_putchar(NACK);
		
		// DEBUG - Version failed
		UART0_putstring("Version failed\n");
		
		// Erase Encrypted Section
		for(int j = 0; j < MAX_PAGE_NUMBER; j++) {
			// Fill pagebuffer with 0xFF
			for(int i = 0; i < SPM_PAGESIZE; i++) {
				pageBuffer[i] = 0xFF;
			}
			
			// Write over Encrypted Section
			program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
		}
		
		wdt_reset();
		
		// Erase Decrypted Section
		for(int j = 0; j < MAX_PAGE_NUMBER; j++) {
			// Fill pagebuffer with 0xFF
			for(int i = 0; i < SPM_PAGESIZE; i++) {
				pageBuffer[i] = 0xFF;
			}
			
			// Write over Encrypted Section
			program_flash(DECRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
		}
		
		// Reset
		while(1) {
			__asm__ __volatile__("");
		}
	}
	else if(newVersion != 0) {
		// Not DEBUG firmware, update version
		eeprom_update_word(&fw_version, newVersion);
	}
	
	wdt_reset();
	
	// DEBUG - Decrypt successful
	UART0_putstring("Version succeeded\n");
	
	UART1_putchar(ACK);
	
	
	
	
	
	/* STORE MESSAGE */
	for(int j = 1; j < 5; j++) {
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			// Read out message
			pageBuffer[i] = pgm_read_byte_far(DECRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		// Place in correct location
		program_flash(MESSAGE_SECTION + (uint32_t)(j - 1) * SPM_PAGESIZE, pageBuffer);
	}
	
	wdt_reset();
	
	
	
	/* STORE PROGRAM */
	for(int j = 5; j < MAX_PAGE_NUMBER - 1; j++) {
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			// Read out firmware
			pageBuffer[i] = pgm_read_byte_far(DECRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		wdt_reset();
		
		// Place in correct location
		program_flash(APPLICATION_SECTION + (uint32_t)(j - 5) * SPM_PAGESIZE, pageBuffer);
	}
	
	wdt_reset();
	
	
	
	/* ERASE FLASH */
	for(int j = 0; j < MAX_PAGE_NUMBER * 2; j++) {
		// Fill pagebuffer with 0xFF
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = 0xFF;
		}
		
		// Write over Encrypted Section
		program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
	}
	
	wdt_reset();
	
	// DEBUG - Firmware loaded
	UART0_putstring("Firmware loaded\n");
	
	UART1_putchar(ACK);
	
	// Reset and boot
	while(1) {
		__asm__ __volatile__("");
	}
	
}


/**
 * \brief Ensures the firmware is loaded correctly and boots it up.
 * 
 * This function calculates the hash of the firmware section and compares it to
 * the one currently stored.
 *
 * HASH CORRECT   -  The function prints a release message if available, and boots. The
 *					 Watchdog Timer is disabled before boot.
 *
 * HASH INCORRECT -  The function sets a EEPROM flag indicating firmware error and resets.
 *
 */
void boot_firmware(void)
{
    // Start the Watchdog Timer.
    wdt_enable(WDTO_2S);



    /* RELEASE MESSAGE */
    uint8_t cur_byte = pgm_read_byte_far(MESSAGE_SECTION);

	if(cur_byte != 0xFF) {
		// Write out release message to UART0.
		UART0_putstring("Release Message:\n");
		int addr = MESSAGE_SECTION;
	
		while ((cur_byte != 0x00)) {
			cur_byte = pgm_read_byte_far(addr);
			UART0_putchar(cur_byte);
			++addr;
		}
	
		UART0_putchar('\n');
	}
	
	
	
    /* DISABLE WATCHDOG */
    wdt_reset();
    wdt_disable();



    /* JUMP TO FIRMWARE */
    asm ("jmp 0000");
}


/**
 * \brief Programs a page of ATMega1284P flash memory
 *
 * To program flash, you need to access and program it in pages
 * On the atmega1284p, each page is 128 words, or 256 bytes
 *
 * Programing involves four things,
 * 1. Erasing the page
 * 2. Filling a page buffer
 * 3. Writing a page
 * 4. When you are done programming all of your pages, enable the flash
 *
 * You must fill the buffer one word at a time
 *
 *\param page_address Starting address of page to be programmed
 *\param data 256-byte array of data to be programmed
 */
void program_flash(uint32_t page_address, unsigned char *data)
{
    int i = 0;
    uint8_t sreg;

    // Disable interrupts
    sreg = SREG;
    cli();

    boot_page_erase_safe(page_address);

    for(i = 0; i < SPM_PAGESIZE; i += 2)
    {
        uint16_t w = data[i];    // Make a word out of two bytes
        w += data[i+1] << 8;
        boot_page_fill_safe(page_address+i, w);
    }

    boot_page_write_safe(page_address);
    boot_rww_enable_safe(); // We can just enable it after every program too

    //Re-enable interrupts
    SREG = sreg;
}