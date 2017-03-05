#include "eeprom_safe.h"

/** 
 * \brief Reads a key_inverse pair from EEPROM
 *
 * This function uses a lookup table to convert balanced key-inverse
 * pairs to their normal key representation. This is used to prevent
 * attackers from using a lookup table to glean information about the 
 * Hamming distance of the keys during boot. 
 *
 * During compile, each nibble of the values to be encoded is expanded to
 * a full byte, doubling its size. Each individual bit is expanded using
 * the formula below:
 *
 *		0 -> 01
 *		1 -> 10
 *
 * This process can be applied to a full byte, resulting in something like
 * the example below:
 *
 *		00001101 -> 01010101 10100110
 * 
 * Each byte will now hold exactly 4 1's and 4 0's. To use the keys, however,
 * this process must be reversed. This function accomplishes this through use
 * of a lookup table, nibble by nibble.
 *
 * \param dst Pointer to the destination array for the decoded key
 * \param src Pointer to the source of the key in EEPROM
 * \param length Length of the key-inverse pair, in bytes.
 */
void safe_eeprom_read_block(uint8_t* dst, uint8_t* src, uint8_t length)
{
	//length must be at least 2 bytes and must be even
	uint8_t buffer[length];// (uint8_t*)malloc(length);
	//read from eeprom
	volatile uint8_t i = 0;
	eeprom_busy_wait();
	for(i = 0; i < length; i++)
	{
		buffer[i] = (uint8_t)eeprom_read_byte((src + i));
	}
	
	i = 0;
	uint8_t j = 0;

	for(j = 0; j<length; j++)//byte addressable
	{
			
		switch(buffer[j])
		{
			case 0x55:
			dst[i] |= 0x00;
			break;
				
			case 0x56:
			dst[i] |= 0x01;
			break;
			
			case 0x59:
			dst[i] |= 0x02;
			break;
			
			case 0x5A:
			dst[i] |= 0x03;
			break;
				
			case 0x65:
			dst[i] |= 0x04;
			break;	
		
			case 0x66:
			dst[i] |= 0x05;
			break;
							
			case 0x69:
			dst[i] |= 0x06;
			break;
							
			case 0x6A:
			dst[i] |= 0x07;
			break;
				
			case 0x95:
			dst[i] |= 0x08;
			break;
							
			case 0x96:
			dst[i] |= 0x09;
			break;
				
			case 0x99:
			dst[i] |= 0x0A;
			break;
				
			case 0x9A:
			dst[i] |= 0x0B;
			break;
				
			case 0xA5:
			dst[i] |= 0x0C;
			break;
				
			case 0xA6:
			dst[i] |= 0x0D;
			break;
				
			case 0xA9:
			dst[i] |= 0x0E;
			break;
				
			case 0xAA:
			dst[i] |= 0x0F;
			break;
		}
			
		if((j%2)==0)//if j is even
		{
			dst[i] = (dst[i]<<4);//shift left by four bits
		}
			
		else
		{
			i++;
		}
		
				
	}
	
	
	//free(buffer);
	
}