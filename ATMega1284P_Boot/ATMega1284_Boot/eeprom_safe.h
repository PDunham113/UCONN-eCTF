#ifndef _eeprom_safe_h
#define _eeprom_safe_h

#include <avr/eeprom.h>
//#include <String.h>
//#include <stdio.h>
#include <stdint.h>
//#include <stdlib.h>

void safe_eeprom_read_block(uint8_t* dst, uint8_t* src, uint8_t length);

#endif
