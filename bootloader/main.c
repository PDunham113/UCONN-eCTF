/*
 * bootloader.c
 *
 * Created: 2/21/2017 12:01:23 PM
 * Author : Patrick Dunham, Brian Marquis, Cameron Morris, James Steel, Luke Malinowski, Chenglu Jin
 *
 * Info   : ATMega1284P bootloader. It boots, it loads, it does everything!
 *
 * External Hardware:
 *		- Jumper to ground on PINB2, PINB3, PINB4
 *		- UART <-> USB cable on UART0 (DEBUG)
 *		- UART <-> USB cable on UART1 (COMMS)
 *
 */

/**
 * bootloader.c
 *
 * If Port B Pin 2 (PB2 on the ProtoStack board) is pulled to ground the 
 * bootloader will wait for data to appear on UART1 (which will be interpreted
 * as an updated firmware package).
 * 
 * If the PB2 pin is NOT pulled to ground, but 
 * Port B Pin 3 (PB3 on the ProtoStack board) is pulled to ground, then the 
 * bootloader will enter flash memory readback mode. 
 *
 * If the PB3 pin is NOT pulled to ground, but
 * Port B Pin 4 (PB4 on the ProtoStack board) is pulled to ground, then the
 * bootloader will enter bootloader configure mode.
 * 
 * If NEITHER of these pins are pulled to ground, then the bootloader will 
 * execute the application from flash.
 *
 * The load_firmware function details the structure of the firmware upload messages.
 *
 * The readback function details the structure of the readback request messages.
 * 
 * See program_flash() for information on the process of programming the flash memory.
 * Note that if no data is received after 2 seconds, the bootloader will time out and reset.
 *
 */



#define F_CPU 7300000UL
#define BAUD 115200UL

/*** INCLUDES ***/

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "AES_lib.h"
#include "secret_build_output.txt"



/*** FUNCTION DECLARATIONS ***/

// Random Number Generation
uint16_t quickRand(uint16_t* seed);

// Clock Switching
void initTimer0(void);
void enableClockSwitching(void);
void disableClockSwitching(void);
void setFastMode(void);
void setSlowMode(void);

// Bootloader Functionality
void load_firmware(void);
void boot_firmware(void);
void readback(void);
void configure(void);

// Generic
void calcHash(uint8_t* key, uint16_t startPage, uint16_t endPage, uint8_t* hash);
void program_flash(uint32_t page_address, unsigned char *data);



/*** VARIABLES & DEFINITIONS ***/

// Pin Definitions
#define UPDATE_PIN PINB2
#define READBACK_PIN PINB3
#define CONFIGURE_PIN PINB4

// Character Definitions
#define ACK ((unsigned char)0x06)
#define NACK ((unsigned char)0x15)

// Size of common arrays (in bytes)
#define BLOCK_SIZE 16UL
#define KEY_SIZE   32UL
#define READBACK_PASSWORD_SIZE 24UL

// Load Firmware Message Size (in PAGES)
#define LOAD_FIRMWARE_PAGE_NUMBER 126UL

// Readback Request Size (in bytes)
#define READBACK_REQUEST_SIZE 48UL

// Section Start Address Locations (in bytes)
#define APPLICATION_SECTION 0UL * LOAD_FIRMWARE_PAGE_NUMBER * SPM_PAGESIZE
#define MESSAGE_SECTION     1UL * (LOAD_FIRMWARE_PAGE_NUMBER - 6) * SPM_PAGESIZE
#define ENCRYPTED_SECTION	1UL * LOAD_FIRMWARE_PAGE_NUMBER * SPM_PAGESIZE
#define DECRYPTED_SECTION	2UL * LOAD_FIRMWARE_PAGE_NUMBER * SPM_PAGESIZE
#define BOOTLDR_SECTION		480UL * SPM_PAGESIZE

// Bootloader Control Flags
uint16_t fw_version EEMEM   = 1;
uint8_t  fastClock          = 1;

// Random Number Generation
#define RAND_CLOCK_SWITCH 10

uint16_t randSeed = 6969;

// AES-256 Keys
uint8_t hashKey[KEY_SIZE]			  = H_KEY;
uint8_t readbackHashKey[KEY_SIZE]     = RBH_KEY;
uint8_t firmwareKey[KEY_SIZE]	      = FW_KEY;
uint8_t readbackKey[KEY_SIZE]		  = RB_KEY;

// Initialization Vectors
uint8_t firmwareIV[BLOCK_SIZE] = FW_IV;
uint8_t readbackIV[BLOCK_SIZE] = RB_IV;

// Passwords
uint8_t readbackPassword[READBACK_PASSWORD_SIZE] = RB_PW;



/*** CODE ***/

int main(void) {
	/*** SETUP & INITIALIZATION ***/
	
	// Calibrate Internal RC Oscillator
	OSCCAL = (OSCCAL / 3) + 40;
	
	// Init UARTs (virtual com port)
	UART1_init();

	UART0_init();
	
	// Enable Timer0 for clock switching
	initTimer0();
	
	wdt_reset();
	MCUSR &= ~(1<<WDRF);
	
	wdt_disable();
		
	// Configure Port B Pins 2 and 3 as inputs.
	DDRB &= ~((1 << UPDATE_PIN) | (1 << READBACK_PIN)|(1 << CONFIGURE_PIN));

	// Enable pullups - give port time to settle.
	PORTB |= (1 << UPDATE_PIN) | (1 << READBACK_PIN) | (1 << CONFIGURE_PIN);
	
	// Enable interrupts for clock switching
	sei();
	
	

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



/*** INTERRUPT SERVICE ROUTINES ***/
ISR(TIMER0_COMPA_vect) {
	if(fastClock) {
		setSlowMode();
	}
	else {
		setFastMode();
	}
	
	OCR0A = 50 + quickRand(&randSeed) % RAND_CLOCK_SWITCH;
}


/*** FUNCTION BODIES ***/

/**
 * \brief Interfaces with host configure tool
 *
 * The procedure followed is outlined below.
 *
 * 1 - The routine waits to receive an ACK from the configure tool.
 *
 * 2 - The bootloader now calculates the hash of the bootloader in memory.
 *
 * 3 - The hash is now sent to the tools over UART1.
 *
 *		IF   CORRECT - The bootloader proceeds to check the EEPROM installation
 *
 *		IF INCORRECT - The bootloader sets a flag and terminates
 *
 * 3 - The routine waits for an ACK from the host tools.
 *
 * 4 - The hash of the EEPROM is now calculated.
 *
 * 5 - That hash is sent over UART1 to the host tools.
 *
 *		IF   CORRECT - The bootloader terminates.
 *
 *		IF INCORRECT - A flag is set and the bootloader terminates.
 *
 */
void configure(void) {
	//uint8_t pageBuffer[SPM_PAGESIZE];
	uint8_t hash[BLOCK_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		
	//Start the Watchdog Timer
	wdt_enable(WDTO_4S);
	
	
	
	/*WAIT FOR ACK*/
	while(!UART1_data_available()) {
		__asm__ __volatile__("");
	}
	
	if(UART1_getchar()==ACK) {
		wdt_reset();
	
	
	
		/* CALCULATE HASH */
		
		enableClockSwitching();
		
		calcHash(hashKey, BOOTLDR_SECTION/SPM_PAGESIZE, BOOTLDR_SECTION/SPM_PAGESIZE + 32, hash);
		
		disableClockSwitching();
	
		wdt_reset();
		
		/* SEND HASH */
		for(int i = 0; i < BLOCK_SIZE; i++)	{
			UART1_putchar(hash[i]);
		}
	
		wdt_reset();
		
		/*WAIT FOR ACK*/
		while(!UART1_data_available()) {
			__asm__ __volatile__("");
		}
	
		//reset Watchdog Timer
		wdt_reset();
	
		/*PC WILL ACK IF CORRECT*/
		while(1){__asm__ __volatile__("");}		//wait for reset
	}

}



/**
 * \brief Interfaces with host readback tool.
 *
 * This function allows the factory to read back sections of Application Flash.
 * The tool DOES NOT read from EEPROM or Bootloader Flash.
 * The 48-byte readback request is sent in the following format:
 *
 * -----32 Bytes------ ------_16 Bytes-------   
 *
 * [Encrypted Request] [Readback Request MAC]
 * 
 * The Encrypted Request encrypted using AES-256 in CFB mode, with a specific
 * Readback Key and Readback Initialization Vector. It is in the following format:
 *
 * ------24 Bytes----- ----4 Bytes---- ---4 Bytes---
 *
 * [Readback Password] [Start Address] [End Address]
 *
 * Although the addresses are byte-addressable, this function will dump flash pages
 * (each of which is 256 bytes). The function will begin returning the page containing
 * the starting address, and finish returning the page returning the ending address.
 * Each page is encrypted using the same key and IV and sent back to the PC.
 * 
 * The procedure followed is outlined below.
 * 
 * 1 - The encrypted readback request is received
 *
 * 2 - The CBC-MAC of the encrypted readback request is calculated, and compared to the
 *	   CBC-MAC sent.
 *
 *		IF   CORRECT - The bootloader proceeds with the Readback Request
 *
 *		IF INCORRECT - The bootloader terminates
 *
 * 3 - The readback request is decrypted using the Readback Key and IV
 *
 * 4 - The readback password is checked versus the password stored in the bootloader
 *
 *		IF   CORRECT - The bootloader proceeds with the readback request
 *
 *		IF INCORRECT - The bootloader terminates
 *
 * 5 - The bootloader reads in the start address and end address and converts them to
 *	   start page and end page. The end page is truncated to the page before the
 *	   BOOTLOADER_SECTION. 
 *
 * 6 - The bootloader begins reading the flash data a page at a time. Each page is encrypted
 *	   using AES-256 in CFB mode using the Readback Key and IV before being sent to PC.
 *
 */
void readback(void)
{
	uint32_t startAddress = 0;
	uint32_t size         = 0;
	uint16_t startPage    = 0;
	uint16_t endPage      = 0;
	
	uint8_t  readbackRequest[READBACK_REQUEST_SIZE];
	uint8_t  decryptdRequest[READBACK_REQUEST_SIZE - BLOCK_SIZE];
	uint8_t  hash[BLOCK_SIZE]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	uint8_t blockBuffer[BLOCK_SIZE];
	uint8_t pageBuffer[SPM_PAGESIZE];
	uint8_t encryptedBuffer[SPM_PAGESIZE];
	
	aes256_ctx_t ctx;
	
    // Start the Watchdog Timer
    wdt_enable(WDTO_4S);
	
	// Wait for data
	while(!UART1_data_available()) {
		__asm__ __volatile__("");
	}
	
	/* GET READBACK REQUEST */
	
	for(int i = 0; i < READBACK_REQUEST_SIZE; i++) {
		readbackRequest[i] = UART1_getchar();
	}
	
	wdt_reset();

	/* COMPUTE HASH */
	
	enableClockSwitching();
		
	hashCBC(readbackHashKey, readbackRequest, hash, READBACK_REQUEST_SIZE - BLOCK_SIZE);
	
	disableClockSwitching();

    wdt_reset();
		
	/* CHECK HASH */
	
	for(int i = 0; i < BLOCK_SIZE; i++) {
		if(hash[i] != readbackRequest[READBACK_REQUEST_SIZE - BLOCK_SIZE + i]) {
			UART1_putchar(NACK);
			// Reset
			while(1) {__asm__ __volatile__("");}
		}
	}
		
	wdt_reset();
	
	
	
	/* DECRYPT MESSAGE */
	
	enableClockSwitching();
	
	for(int i = 0; i < (READBACK_REQUEST_SIZE - BLOCK_SIZE); i += BLOCK_SIZE) {
		if(i == 0) {
			strtDecCFB(readbackKey, &readbackRequest[i], readbackIV, &ctx, &decryptdRequest[i]);
		}
		else {
			contDecCFB(&ctx, &readbackRequest[i], &readbackRequest[i - BLOCK_SIZE], &decryptdRequest[i]);
		}
	}	
	
	wdt_reset();
	
	disableClockSwitching();
		
	/* CHECK PASSWORD */
	
	for(int i = 0; i < READBACK_PASSWORD_SIZE; i++) {
		if(readbackPassword[i] != decryptdRequest[i]) {
			UART1_putchar(NACK);						
			// Reset
			while(1) {__asm__ __volatile__("");}
		} 
	}
	
	wdt_reset();
	
	
	
	/* GATHER PARAMETERS */
	
	enableClockSwitching();
	
	// Gather start address
	for(int i = 0; i < 4; i++) {
		startAddress |= (decryptdRequest[READBACK_PASSWORD_SIZE + i] << (8 * (3-i)));
	}
	
	// Gather size
	for(int i = 0; i < 4; i++) {
		size |= (decryptdRequest[READBACK_PASSWORD_SIZE + 4 + i] << (8 * (3-i)));
	}
	
	// Convert to start page and end page
	startPage = (startAddress / SPM_PAGESIZE);
	endPage   = (startAddress + size - 1) / SPM_PAGESIZE;
	
	// If start page is outside application section, truncate
	if(startPage > ((MESSAGE_SECTION / SPM_PAGESIZE) - 1)) {
		startPage = (MESSAGE_SECTION / SPM_PAGESIZE) - 1;
	}
	
	// If end page is outside application section, truncate
	if(endPage > ((MESSAGE_SECTION / SPM_PAGESIZE) - 1)) {
		endPage = (MESSAGE_SECTION / SPM_PAGESIZE) - 1;
	}
	
	wdt_reset();
	

		
	/* ENCRYPT & SEND FLASH */	
	
	// Unlike the other for loops in this format, this one terminates if j > endPage, not if j >= endPage.
	// Pay attention to this.
	for(int j = startPage; j <= endPage; j++) {
		
		// Reads page
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = pgm_read_byte_far((uint32_t)j * SPM_PAGESIZE + i);
		}
		
		wdt_reset();
		
		enableClockSwitching();
		
		// Encrypts page
		for(int i = 0; i < SPM_PAGESIZE; i += BLOCK_SIZE) {
			if((j == startPage) && (i == 0)) {
				strtEncCFB(readbackKey, &pageBuffer[i], readbackIV, &ctx, &encryptedBuffer[i]);
			}
			else if(i == 0) {
				contEncCFB(&ctx, &pageBuffer[i], blockBuffer, &encryptedBuffer[i]);
			}
			else {
				contEncCFB(&ctx, &pageBuffer[i], &encryptedBuffer[i - BLOCK_SIZE], &encryptedBuffer[i]);
			}
		}
			
		// Save data for next page
		for(int i = 0; i < BLOCK_SIZE; i++) {
			blockBuffer[i] = encryptedBuffer[SPM_PAGESIZE - BLOCK_SIZE + i];
		}
		
		disableClockSwitching();
		
		// Print data
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			UART1_putchar(encryptedBuffer[i]);
		}

	}

	wdt_reset();

	// Reset and boot
    while(1) {
		 __asm__ __volatile__("");
	}
}



/**
 * \brief Loads a new firmware image and release message
 * 
 * This function securely loads a new firmware image onto flash. Firmware is encrypted using
 * AES-256 in CFB Mode, and sent one page at a time. It is in the following format:
 *
 * --------32000 Bytes-------- ----256 Bytes-----
 *
 * [Encrypted Firmware Update] [Message MAC Page]
 *
 * The Encrypted Firmware Update section is broken into the following pages:
 *
 * --256 Bytes--- --30720 Bytes--- --1024 Bytes---
 *
 * [Version Page] [Firmware Pages] [Message Pages]
 *
 * The version page is constructed in the following format:
 *
 * -----2 Bytes---- ---254 Bytes----
 *
 * [Version Number] [Random Padding]
 *
 * Each Firmware Page consists of 256 bytes of raw flash. These are obtained via the PC stripping
 * the .hex file generated from the firmware Makefile. Address offsets are added, and blank Flash
 * in the Application Section will be written to 0xFF.
 *
 * The Message Page is 1024 bytes input by the user at FW_PROTECT time. The string is null
 * terminated, and empty bytes in the Message Section will be written to 0xFF
 * 
 * The Message MAC page is constructed in the following format:
 *
 * --16 Bytes--- ---240 Bytes----
 *
 * [Message MAC] [Random Padding]
 *
 * The procedure followed is outlined below.
 * 
 * 1 - The encrypted firmware image is loaded into the ENCRYPTED_SECTION of flash.
 *
 * 2 - The CBC-MAC of the encrypted firmware image is computed, and compared to the
 *	   CBC-MAC sent.
 *
 *		IF   CORRECT - The bootloader proceeds with the firmware upload.
 *
 *		IF INCORRECT - The bootloader erases ENCRYPTED_SECTION and terminates.
 *
 * 3 - The firmware image is decrypted and stored in the DECRYPTED_SECTION of flash.
 *
 * 4 - The version number is checked versus the current version, and updated in EEPROM
 *
 *		IF   CORRECT - The bootloader proceeds with the firmware upload.
 *
 *		IF INCORRECT - The bootloader erases ENCRYPTED_SECTION and DECRYPTED_SECTION and
 *					   terminate.
 *
 * 5 - The release message is written to the MESSAGE_SECTION
 *
 * 6 - The firmware is written to the APPLICATION_SECTION
 *
 * 7 - The bootloader erases ENCRYPTED_SECTION and DECRYPTED_SECTION and terminates
 *
 */
void load_firmware(void) {
	uint8_t pageBuffer[SPM_PAGESIZE];
	uint8_t decryptedBuffer[SPM_PAGESIZE];
	uint8_t blockBuffer[BLOCK_SIZE];
	
	uint8_t hash[BLOCK_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	uint16_t currentVersion = eeprom_read_word(&fw_version);
	uint16_t newVersion     = 0x0001;
	
	aes256_ctx_t ctx;
	
	// Start Watchdog Timer
	wdt_enable(WDTO_4S);
	
	
	
	/* GET UART DATA, CALCULATE HASH */
	
	for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER; j++) {
		
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
 		
		// Get ready for next page
		UART1_putchar(ACK);
		
		// Reset WDT
		wdt_reset();
	}
	
	enableClockSwitching();
	
	calcHash(hashKey, ENCRYPTED_SECTION / SPM_PAGESIZE, ENCRYPTED_SECTION / SPM_PAGESIZE + LOAD_FIRMWARE_PAGE_NUMBER - 1, hash);
	
	wdt_reset();
	
	disableClockSwitching();


	
	/* CHECK HASH */
	
	for(int i = 0; i < BLOCK_SIZE; i++) {
		
		// If hash is wrong, erase and reset
		if(hash[i] != pageBuffer[i]) {
			
			// Send NACK
			UART1_putchar(NACK);
			
			// DEBUG - Tell us hash failed
			UART0_putstring("Wrong H\n");
			
			// Fill pageBuffer with 0xFF
			for(int i = 0; i < SPM_PAGESIZE; i++) {
				pageBuffer[i] = 0xFF;
			}
			
			wdt_reset();
			
			// Write over Encrypted Section
			for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER; j++) {
				program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
				
				wdt_reset();
			}
			
			// Reset
			while(1) {
				__asm__ __volatile__("");
			}
		}
	}
	
	wdt_reset();
	
	// IS THIS NEEDED?
	//UART1_putchar(ACK);
	
	// DEBUG - Tell us hash succeeded
	//UART0_putstring("Good H\n");
	
	
	
	/* DECRYPT */
	
	for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER; j++) {
		
		// Reads page from flash
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = pgm_read_byte_far(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE + i);
		}
		
		wdt_reset();
		
		enableClockSwitching();
		
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
		
		disableClockSwitching();
		
		// Writes data to Decrypted Section
		program_flash(DECRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, decryptedBuffer);
		
		wdt_reset();
	}
	
	wdt_reset();
	
	// IS THIS NEEDED?
	// UART1_putchar(ACK);
	
	
	
	/* CHECK VERSION */
	
	for(int j = 0; j < 16; j++) {
	
		// Read version number from Decrypted Section
		newVersion = pgm_read_word_far(DECRYPTED_SECTION);
	
	
		// Compare versions
		if((newVersion != 0) && (newVersion < currentVersion)) {
		
			// Firmware Too Old
			UART1_putchar(NACK);
		
			// DEBUG - Version failed
			UART0_putstring("VN Fail\n");
		
			// Erase Encrypted Section
			for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER; j++) {
			
				// Fill page buffer with 0xFF
				for(int i = 0; i < SPM_PAGESIZE; i++) {
					pageBuffer[i] = 0xFF;
				}
			
				// Write over Encrypted Section
				program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
			}
		
			wdt_reset();
		
			// Erase Decrypted Section
			for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER; j++) {
			
				// Fill page buffer with 0xFF
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
	}
	
	wdt_reset();
	
	// DEBUG - Decrypt successful
	//UART0_putstring("Good DCPT\n");
	
	// IS THIS NEEDED?
	//UART1_putchar(ACK);
	
	
	
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
	
	for(int j = 5; j < LOAD_FIRMWARE_PAGE_NUMBER - 1; j++) {
		
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
	
	for(int j = 0; j < LOAD_FIRMWARE_PAGE_NUMBER * 2; j++) {
		
		// Fill page buffer with 0xFF
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = 0xFF;
		}
		
		// Write over Encrypted Section
		program_flash(ENCRYPTED_SECTION + (uint32_t)j * SPM_PAGESIZE, pageBuffer);
	}
	
	wdt_reset();
	
	// DEBUG - Firmware loaded
	UART0_putstring("FW Up\n");
	
	UART1_putchar(ACK);
	
	
	
	// Reset and boot
	while(1) {
		__asm__ __volatile__("");
	}
	
}



/**
 * \brief Ensures the firmware is loaded correctly and boots it up.
 *
 */
void boot_firmware(void)
{
	
    // Start the Watchdog Timer.
    wdt_enable(WDTO_4S);

	

    /* RELEASE MESSAGE */
	
    uint8_t cur_byte = pgm_read_byte_far(MESSAGE_SECTION);

	// If there is a release message...
	if(cur_byte != 0xFF) {
		
		// Write out release message to UART0.
		int addr = MESSAGE_SECTION;
	
		while ((cur_byte != 0x00) && (addr < (MESSAGE_SECTION + 4 * SPM_PAGESIZE))) {
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

    for(i = 0; i < SPM_PAGESIZE; i += 2) {
		// Make a word out of two bytes
        uint16_t w = data[i];    
        w += data[i+1] << 8;
		
		// Write to page buffer
        boot_page_fill_safe(page_address+i, w);
    }

    boot_page_write_safe(page_address);
	
	// We can just enable it after every program too
    boot_rww_enable_safe(); 

    //Re-enable interrupts if needed
    SREG = sreg;
}



/**
 * \brief Calculates a hash of a memory section
 *
 * This function calculates the CBC-MAC hash of a section in flash. The section is
 * indexed by pages, where 1 page = 256 bytes.
 *
 * The hash array fed to this function MUST be filled with zeros initially, or the hashing will not
 * work correctly. The startPage parameter refers to the first page to be hashed. The endPage
 * parameter does not refer to the last page to be hashed, but to the first page to NOT hash.
 *
 * \param key Pointer to 32-byte array containing the AES-256 key.
 * \param startPage Starting 256-byte page of memory to hash (this page WILL be hashed)
 * \param endPage Ending 256-byte page of memory to hash (this page will NOT be hashed)
 * \param hash Pointer to a 16-byte hash array. Must be initialized to all zeros.
 */
void calcHash(uint8_t* key, uint16_t startPage, uint16_t endPage, uint8_t* hash) {
	uint8_t pageBuffer[SPM_PAGESIZE];
	
	
	for(int j = startPage; j < endPage; j++) {
		
		// Read page to buffer
		for(int i = 0; i < SPM_PAGESIZE; i++) {
			pageBuffer[i] = pgm_read_byte_far((uint32_t)j * SPM_PAGESIZE + i);
		}
		
		wdt_reset();
		
		// Add to hash
		hashCBC(key, pageBuffer, hash, SPM_PAGESIZE);
		
	}
}



/**
 * \brief Initialize Timer0 for clock switching
 *
 */
void initTimer0(void) {
	// Configure to CTC Mode
	TCCR0A = (1 << WGM01);
	TIMSK0 = (1 << OCIE0A);
	
	// Overflow at a random amount
	OCR0A = 50 + quickRand(&randSeed) % RAND_CLOCK_SWITCH;
}



/**
 * \brief Starts Timer0, enabling Clock Switching
 *
 */
void enableClockSwitching(void) {
	TCCR0B = (1 << CS02)|(1 << CS00); // Enable /1024 divider
}


/**
 * \brief Stops Timer0, disabling Clock Switching
 *
 */
void disableClockSwitching(void) {
	TCCR0B = 0;
	setFastMode();
}



/** 
 * \brief Sets clock to fast mode (/1 Prescaler)
 *
 */
void setFastMode(void) {
	// Removes clock divisor
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	
	// Updates Timer 0 speed
	TCCR0B |= (1<<CS00);
	
	fastClock = 1;
}



/** 
 * \brief Sets clock to slow mode (/8 Prescaler)
 *
 */
void setSlowMode(void) {
	// Sets /8 clock divisor
	CLKPR = (1<<CLKPCE);
	CLKPR = (1<<CLKPS1)|(1<<CLKPS0);
	
	// Updates Timer 0
	TCCR0B &= ~(1<<CS00);
	
	fastClock = 0;
}

/** 
 * \brief Generates a pseudo random number using an LSFR
 * 
 * Uses a 16-bit maximal state Galois Linear Feedback Shift Register (LFSR) to create
 * pseudo random numbers. These will NOT pass any sort of random number test, but should
 * be relatively unpredictable. This also uses EXACTLY 37 instructions each iteration,
 * unlike the C rand() implementation, which uses a varying number of instructions (on the 
 * order of 800) each iteration. This makes this random number generator more difficult to
 * notice via side channel analysis, and drastically speeds up runtime. It requires the seed
 * fed as a pointer, unlike the C rand() function, but returns the random number for ease of
 * use.
 *
 * \param seed Pointer to 16-bit PRNG seed Must not be 0
 * \return 16-bit random number.
 */
uint16_t quickRand(uint16_t* seed) {
	*seed >>= 1;
	
	uint8_t lsb = *seed & 1;
	
	*seed ^= (-lsb) & 0xB400u;
	
	return *seed;
}