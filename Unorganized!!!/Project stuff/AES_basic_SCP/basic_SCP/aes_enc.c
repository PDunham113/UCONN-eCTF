/* aes_enc.c */
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
 * \file     aes_enc.c
 * \email    bg@nerilex.org
 * \author   Daniel Otte 
 * \date     2008-12-30
 * \license  GPLv3 or later
 * 
 */

#include <stdint.h>
#include <string.h>
#include "aes.h"
#include "aes_sbox.h"
#include "aes_enc.h"
#include <avr/pgmspace.h>
#include <stdlib.h>
#define NUM_DUMMY_OP 5

void aes_shiftcol(void *data, uint8_t shift)
{
    uint8_t tmp[4];
    tmp[0] = ((uint8_t*) data)[0];
    tmp[1] = ((uint8_t*) data)[4];
    tmp[2] = ((uint8_t*) data)[8];
    tmp[3] = ((uint8_t*) data)[12];
    ((uint8_t*) data)[0] = tmp[(shift + 0) & 3];
    ((uint8_t*) data)[4] = tmp[(shift + 1) & 3];
    ((uint8_t*) data)[8] = tmp[(shift + 2) & 3];
    ((uint8_t*) data)[12] = tmp[(shift + 3) & 3];
}

//#define xtime(x)   ((x<<1) ^ (((x>>7) & 1) * 0x1b))
uint8_t xtime (uint8_t x)
{
	uint8_t res = ((x<<1) ^ (((x>>7) & 1) * 0x1b));
	return res;
}


static
void aes_enc_round(aes_cipher_state_t *state, const aes_roundkey_t *k, uint8_t rounds)
{
    uint8_t  tmp[16], t;
    volatile uint8_t i;
	uint8_t temp;	
	uint8_t dummy_before, j;
	uint8_t dummy_value;
	dummy_value = rand()&0xff;
	dummy_before = rand()%NUM_DUMMY_OP;
	uint8_t shuffle_index;
	shuffle_index = rand()%16;
	//fill tmp with random numbers
	for (i=0; i <16; i++)
	{
		tmp[i] = rand()&0xff;
	}
    /* subBytes */
	//dummy operations for round 1,  2 and 13
	if (rounds == 1 || rounds == 2 || rounds == 13)
	{
		for (j = 0; j < dummy_before; j++)
		{
			shuffle_index++;
			shuffle_index = shuffle_index&0xf;
			dummy_value = pgm_read_byte(aes_sbox + dummy_value);
		}
	}
	//shuffling
	for (i = 0; i < 16; ++i) {
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		tmp[shuffle_index] = pgm_read_byte(aes_sbox + state->s[shuffle_index]);
	}
	//dummy operations for round 1, 2, and 13
	if (rounds == 1 || rounds == 2 || rounds ==13)
	{
		for (j = dummy_before; j < NUM_DUMMY_OP; j++)
		{
			shuffle_index++;
			shuffle_index = shuffle_index&0xf;
			dummy_value = pgm_read_byte(aes_sbox + dummy_value);
		}
	}
	//mask for linear part of AES
	uint8_t mask[16];
	for (i = 0; i < 16; i++)
	{
		mask[i] = rand()%16;
		tmp[i] = tmp[i] ^ mask[i];
	}
    /* shiftRows */
    aes_shiftcol(tmp + 1, 1);
	aes_shiftcol(mask + 1, 1);
    aes_shiftcol(tmp + 2, 2);
	aes_shiftcol(mask + 2, 2);
    aes_shiftcol(tmp + 3, 3);
	aes_shiftcol(mask + 3, 3);
	
    /* mixColums */
    for ( i = 0; i < 4; ++i) {
        t = tmp[4 * i + 0] ^ tmp[4 * i + 1] ^ tmp[4 * i + 2] ^ tmp[4 * i + 3];
        temp = xtime(tmp[4*i+0]^tmp[4*i+1]);
		temp ^= tmp[4 * i + 0];
		temp ^=  t;
		state->s[4 * i + 0] = temp; 
		temp = xtime(tmp[4*i+1]^tmp[4*i+2])
		^ tmp[4 * i + 1]
		^ t;
		state->s[4 * i + 1] = temp;
		temp = xtime(tmp[4*i+2]^tmp[4*i+3])
		^ tmp[4 * i + 2]
		^ t;
        state->s[4 * i + 2] = temp;
		temp = xtime(tmp[4*i+3]^tmp[4*i+0])
		^ tmp[4 * i + 3]
		^ t;
           state->s[4 * i + 3] = temp;        
    }
	/* mixColums */
	for ( i = 0; i < 4; ++i) {
		t = mask[4 * i + 0] ^ mask[4 * i + 1] ^ mask[4 * i + 2] ^ mask[4 * i + 3];
		temp = xtime(mask[4*i+0]^mask[4*i+1]);
		temp ^= mask[4 * i + 0];
		temp ^=  t;
		tmp[4 * i + 0] = temp;
		temp = xtime(mask[4*i+1]^mask[4*i+2]) ^ mask[4 * i + 1] ^ t;
		tmp[4 * i + 1] = temp;
		temp = xtime(mask[4*i+2]^mask[4*i+3]) ^ mask[4 * i + 2] ^ t;
		tmp[4 * i + 2] = temp;
		temp = xtime(mask[4*i+3]^mask[4*i+0]) ^ mask[4 * i + 3] ^ t;
		tmp[4 * i + 3] = temp;	    
	    }

    /* addKey */
	//shuffling
	shuffle_index = rand()%16;
    for (i = 0; i < 16; ++i) {
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
        state->s[shuffle_index] ^= k->ks[shuffle_index];
		state->s[shuffle_index] ^= tmp[shuffle_index];
    }
}

static void aes_enc_lastround(aes_cipher_state_t *state, const aes_roundkey_t *k)
{
    uint8_t i;
	uint8_t tmp[16];
	uint8_t dummy_before, j;
	uint8_t dummy_value;
	dummy_value = rand()&0xff;
	dummy_before = rand()%NUM_DUMMY_OP;
	uint8_t shuffle_index;
	shuffle_index = rand()%16;
	/* subBytes */
	//dummy operations
	for (j = 0; j < dummy_before; j++)
	{
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		dummy_value = pgm_read_byte(aes_sbox + dummy_value);
	}
	//shuffling
	for (i = 0; i < 16; ++i) {
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		tmp[shuffle_index] = pgm_read_byte(aes_sbox + state->s[shuffle_index]);
	}
	//dummy operations
	for (j = dummy_before; j < NUM_DUMMY_OP; j++)
	{
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		dummy_value = pgm_read_byte(aes_sbox + dummy_value);
	}
	//mask for linear part of AES
	uint8_t mask[16];
	for (i = 0; i < 16; i++)
	{
		mask[i] = rand()%16;
		tmp[i] = tmp[i] ^ mask[i];
	}
	/* shiftRows */
	aes_shiftcol(tmp + 1, 1);
	aes_shiftcol(mask + 1, 1);
	aes_shiftcol(tmp + 2, 2);
	aes_shiftcol(mask + 2, 2);
	aes_shiftcol(tmp + 3, 3);
	aes_shiftcol(mask + 3, 3);

    /* addKey */
	//Dummy operations
	uint8_t dummy_mask[16];
    shuffle_index = rand()%16;
	dummy_before = rand()%NUM_DUMMY_OP;
	for (i = 0; i < 16; i++)
	{
		dummy_mask[i] = rand()%16;
	}
	for (j = 0; j <dummy_before; j++)
	{
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		dummy_value = tmp[shuffle_index];
		dummy_value ^= dummy_mask[shuffle_index];
		dummy_value ^= mask[shuffle_index];
	}
    //shuffling
    for (i = 0; i < 16; ++i) {
	    shuffle_index++;
	    shuffle_index = shuffle_index&0xf;
		state->s[shuffle_index] = tmp[shuffle_index];
	    state->s[shuffle_index] ^= k->ks[shuffle_index];
	    state->s[shuffle_index] ^= mask[shuffle_index];
    }
	for (j = NUM_DUMMY_OP; j <NUM_DUMMY_OP; j++)
	{
		shuffle_index++;
		shuffle_index = shuffle_index&0xf;
		dummy_value = tmp[shuffle_index];
		dummy_value ^= dummy_mask[shuffle_index];
		dummy_value ^= mask[shuffle_index];
	}
}

void aes_encrypt_core(aes_cipher_state_t *state, const aes_genctx_t *ks, uint8_t rounds)
{
    uint8_t i;
	uint8_t mask[16];
	uint8_t shuffle_index;
	uint8_t dummy_mask[NUM_DUMMY_OP], dummy_value[NUM_DUMMY_OP];
	uint8_t dummy_before, j;
	shuffle_index = rand()%16;
	//mask for round 0
	for (i = 0; i< 16; i++)
	{
		mask[i] = rand()&0xff;
	}
	for (i = 0; i <NUM_DUMMY_OP; i++)
	{
		dummy_mask[i] = rand()&0xff;
		dummy_value[i] = rand()&0xff;		
	}
	//dummy operation
	dummy_before = rand()%NUM_DUMMY_OP;		
	shuffle_index = rand()%16;
	for (j = 0; j <dummy_before; j++)
	{
		shuffle_index++;
		shuffle_index &= 0xf;
		dummy_value[j] ^= dummy_mask[j];
		dummy_value[j] ^= mask[j];
		dummy_value[j] ^= dummy_mask[j];
	}	
	//shuffle round 0 ARK
	for (i = 0; i < 16; ++i) {
		shuffle_index++;
		shuffle_index &= 0xf;
		state->s[shuffle_index] ^= mask[i];
		state->s[shuffle_index] ^= ks->key[0].ks[shuffle_index];
		state->s[shuffle_index] ^= mask[i];
	}
	//dummy operation
	for (; j <NUM_DUMMY_OP; j++)
	{
		shuffle_index++;
		shuffle_index &= 0xf;
		dummy_value[j] ^= dummy_mask[j];
		dummy_value[j] ^= mask[j];
		dummy_value[j] ^= dummy_mask[j];
	}
    i = 1;
    for (; rounds > 1; --rounds) {
        aes_enc_round(state, &(ks->key[i]), i);
        ++i;
    }
    aes_enc_lastround(state, &(ks->key[i]));
}
