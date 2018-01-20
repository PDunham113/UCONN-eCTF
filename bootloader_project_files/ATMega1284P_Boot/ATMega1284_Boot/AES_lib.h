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



/* CFB MODE ENCRYPTION */

// Encrypts entire message. Only used if entire message is in RAM.
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);

// Encrypts message block by block. Far more flexible.
void strtEncCFB(uint8_t* key, uint8_t* firstBlockPlaintext, uint8_t* IV, aes256_ctx_t* ctx, uint8_t* firstBlockCiphertext);
void contEncCFB(aes256_ctx_t* ctx, uint8_t* nextBlockPlaintext, uint8_t* prevBlockCiphertext, uint8_t* nextBlockCiphertext);



/* CFB MODE DECRYPTION */

// Decrypts entire message. Only used if entire message is in RAM.
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size);

// Decrypts message block by block. Far more flexible.
void strtDecCFB(uint8_t* key, uint8_t* firstBlockCiphertext, uint8_t* IV, aes256_ctx_t* ctx, uint8_t* firstBlockPlaintext);
void contDecCFB(aes256_ctx_t* ctx, uint8_t* nextBlockCiphertext, uint8_t* prevBlockCiphertext, uint8_t* nextBlockPlaintext);



/* CBC-MAC */

// Adds a RAM buffer to a MAC/hash. Hash parameter must be initialized to zero for a new hash.
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* hash, uint16_t size);

#endif /* AES_LIB_H_ */