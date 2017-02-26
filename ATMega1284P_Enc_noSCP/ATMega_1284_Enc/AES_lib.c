/*
 * AES_lib.c
 *
 * Created: 2/26/2017 12:49:57 PM
 *  Author: Patrick Dunham
 */ 

/**
 * \file    AES_lib.c 
 * \author  Patrick Dunham
 * \date    2017-02-26
 * \license GPLv3 or later
 */

#include "AES_lib.h"
#include "AES_lib/aes.h"



/** 
 * \brief Encrypts data in-place using AES-256 in CFB Mode
 * 
 * This method uses a standard AES-256 encryption kernel to encrypt data in CFB
 * Mode. This essentially turns the block cipher into a self-correcting stream
 * cipher. In addition, the corresponding decryption method can also use the 
 * AES-256 encryption kernel, saving code space. The code will be encrypted block
 * by block and stored in the same memory space used for the plaintext. This
 * results in drastic memory savings. The data array byte count MUST be divisible
 * by 16. This can be achieved through padding.
 * 
 * \param key Pointer to 32-byte array containing the AES-256 key.
 * \param data Pointer to data array. Begins as plaintext, ends as ciphertext.
 * \param IV Pointer to a 16-byte random Initialization Vector
 * \param size Size in bytes of data array. Must be divisible by 16.
 */
void encCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size) {
	// Create address counter and block buffer
	uint16_t	 _address = 0;
	uint8_t		 _buffer[16];
	aes256_ctx_t ctx;
	
	// Compute AES-256 Keyschedule
	aes256_init(key, &ctx);


	
	// Copy IV to buffer for First Round
	for(uint8_t i = 0; i < 16; i++) {
		_buffer[i] = IV[i];
	}	
	
	// Encryption Rounds
	while(_address < size) {
		
		// Encrypt buffer in place
		//aes256_enc_single(key, _buffer);
		aes256_enc(_buffer, &ctx);
		
		// XOR plaintext with encrypted buffer. Store in place.
		for(uint8_t i = 0; i < 16; i++) {
			data[_address + i] ^= _buffer[i];
		}
		
		// Copy next block to buffer
		for(uint8_t i = 0; i < 16; i++) {
			_buffer[i] = data[_address + i];
		}
		
		// Move address to next block
		_address += 16;
	}
}



/** 
 * \brief Decrypts data in-place using AES-256 in CFB Mode
 * 
 * This method uses a standard AES-256 encryption kernel to decrypt data in CFB
 * Mode. This essentially turns the block cipher into a self-correcting stream
 * cipher. Unlike other block cipher modes, the decryption method can also use the
 * AES-256 encryption kernel, saving code space. The code will be decrypted block
 * by block and stored in the same memory space used for the ciphertext. This
 * results in drastic memory savings. The data array byte count MUST be divisible
 * by 16. This should have been accounted for in the encryption function.
 * 
 * \param key Pointer to 32-byte array containing the AES-256 key.
 * \param data Pointer to data array. Begins as ciphertext, ends as plaintext.
 * \param IV Pointer to a 16-byte random Initialization Vector
 * \param size Size in bytes of data array. Must be divisible by 16.
 */
void decCFB(uint8_t* key, uint8_t* data, uint8_t* IV, uint16_t size) {
	// Create address counter and block buffer
	uint16_t     _address = 0;
	uint8_t      _buffer1[16];
	uint8_t      _buffer2[16];
	aes256_ctx_t ctx;
	
	// Compute AES-256 Keyschedule
	aes256_init(key, &ctx);
	
	
	// Copy IV to buffer1 for First Round
	for(uint8_t i = 0; i < 16; i++) {
		_buffer1[i] = IV[i];
	}
	
	// Encryption Rounds
	while(_address < size) {
		
		// Encrypt buffer1 in place
		aes256_enc(_buffer1, &ctx);
		
		// Copy ciphertext to buffer2
		for(uint8_t i = 0; i < 16; i++) {
			_buffer2[i] = data[_address + i];
		}
		
		// XOR buffer1 with ciphertext. Store in data
		for(uint8_t i = 0; i < 16; i++) {
			data[_address + i] = _buffer1[i] ^ _buffer2[i];
		}
		
		// Copy buffer2 to buffer1
		for(uint8_t i = 0; i < 16; i++) {
			_buffer1[i] = _buffer2[i];
		}
		
		_address += 16;
	}
}



/** 
 * \brief Generates a 128-bit hash using AES-256 CBC-MAC
 * 
 * This method uses a standard AES-256 encryption kernel to generate a 128-byte
 * Message Authentication Code (MAC) by running the cipher in CBC Mode. Only the
 * last encrypted block is saved. This hash must be used for messages with fixed
 * sizes only, and the encryption key must be different than the one used for the
 * message encryption. The data is left untouched, unlike with @code encCFB() and
 * @code decCFB(). The hash parameter must be initialized to zero before calling
 * this function.
 * 
 * \param key Pointer to 32-byte array containing the AES-256 key.
 * \param data Pointer to data array. Begins as ciphertext, ends as plaintext.
 * \param hash Pointer to a 16-byte hash array. Must be initialized to all zeros.
 * \param size Size in bytes of data array. Must be divisible by 16.
 */
void hashCBC(uint8_t* key, uint8_t* data, uint8_t* hash, uint16_t size) {
	uint16_t     _address = 0;
	aes256_ctx_t ctx;
	
	// Compute AES-256 Keyschedule
	aes256_init(key, &ctx);
	
	// Hashing Rounds
	while(_address < size) {
		// XOR current hash with plaintext
		for(uint8_t i = 0; i < 16; i++) {
			hash[i] ^= data[i];
		}
		
		// Encrypt current hash in place
		aes256_enc(hash, &ctx);
		
		// Increment address
		_address += 16;


	}
	
}