/* seed.h */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2006-2015 Daniel Otte (bg@nerilex.org)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * \file	seed.h
 * \author	Daniel Otte 
 * \date	2007-06-1
 * \brief 	declarations for seed
 * \par License	
 * GPL
 * 
 */
#ifndef SEED_H_
#define SEED_H_

#include <stdint.h>
/** \typedef seed_ctx_t
 * \brief SEED context
 * 
 * A variable of this type may hold the key material for the SEED cipher. 
 * This context is regulary generated by the 
 * void seed_init(const void * key, seed_ctx_t * ctx) function.
 */
typedef struct{
	uint32_t k[4];
} seed_ctx_t;

/******************************************************************************/

/** \fn void seed_init(const void * key, seed_ctx_t * ctx)
 * \brief initializes context for SEED operation
 * 
 * This function copys the key material into a context variable.
 * 
 * \param key  pointer to the key material (128 bit = 16 bytes)
 * \param ctx  pointer to the context (seed_ctx_t)
 */
void seed_init(const void * key, seed_ctx_t * ctx);

/** \fn void seed_enc(void * buffer,const seed_ctx_t * ctx)
 * \brief encrypt a block with SEED
 * 
 * This function encrypts a block of 64 bits (8 bytes) with the SEED algorithm.
 * The round keys are computed on demand, so the context is modifyed while
 * encrypting but the original stated is restored when the function exits.
 * 
 * \param buffer pointer to the block (64 bit = 8 byte) which will be encrypted
 * \param ctx    pointer to the key material (seed_ctx_t)
 */
void seed_enc(void * buffer, const seed_ctx_t * ctx);


/** \fn void seed_dec(void * buffer, const seed_ctx_t * ctx)
 * \brief decrypt a block with SEED
 * 
 * This function decrypts a block of 64 bits (8 bytes) with the SEED algorithm.
 * The round keys are computed on demand, so the context is modifyed while
 * decrypting but the original stated is restored when the function exits.
 * 
 * \param buffer pointer to the block (64 bit = 8 byte) which will be decrypted
 * \param ctx    pointer to the key material (seed_ctx_t)
 */
void seed_dec(void * buffer, const seed_ctx_t * ctx);

	
#endif /*SEED_H_*/
