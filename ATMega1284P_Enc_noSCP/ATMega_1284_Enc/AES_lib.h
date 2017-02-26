/*
 * AES_lib.h
 *
 * Created: 2/26/2017 12:49:46 PM
 *  Author: Patrick Dunham
 */ 

/**
 * \file    AES_lib.h 
 * \author  Patrick Dunham
 * \date    2017-02-26
 * \license GPLv3 or later
 */


#ifndef AES_LIB_H_
#define AES_LIB_H_

#include "AES_lib/aes.h"



void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* hash, uint16_t size);

#endif /* AES_LIB_H_ */