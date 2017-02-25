/* sha256.c */
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
 * \file		sha256.c
 * \author		Daniel Otte
 * \date		16.05.2006
 *
 * \par License:
 * 	GPL
 *
 * \brief SHA-256 implementation.
 *
 *
 */

#include <stdint.h>
#include <string.h> /* for memcpy, memmove, memset */
#include <avr/pgmspace.h>
#include "sha256.h"

#define LITTLE_ENDIAN

#if defined LITTLE_ENDIAN
#elif defined BIG_ENDIAN
#else
	#error specify endianess!!!
#endif


/*************************************************************************/

static const uint32_t sha256_init_vector[] PROGMEM = {
	0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
    0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL };


/*************************************************************************/

/**
 * \brief \c sh256_init initialises a sha256 context for hashing.
 * \c sh256_init c initialises the given sha256 context for hashing
 * @param state pointer to a sha256 context
 * @return none
 */
void sha256_init(sha256_ctx_t *state){
	state->length = 0;
	memcpy_P(state->h, sha256_init_vector, 8 * 4);
}

/*************************************************************************/

/**
 * rotate x right by n positions
 */
uint32_t rotr32( uint32_t x, uint8_t n){
	return ((x >> n) | (x << (32 - n)));
}


/*************************************************************************/

// #define CHANGE_ENDIAN32(x) (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) | (((x)& 0x00ff0000)>>8))

uint32_t change_endian32(uint32_t x){
	return (  ((x) << 24)
	        | ((x) >> 24)
	        | (((x) & 0x0000ff00UL) << 8)
	        | (((x) & 0x00ff0000UL) >> 8) );
}


/*************************************************************************/

/* sha256 functions as macros for speed and size, cause they are called only once */

#define CH(x,y,z)  (((x) & (y)) ^ ((~(x)) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SIGMA0(x) (rotr32((x), 2) ^ rotr32((x), 13) ^ rotr32((x), 22))
#define SIGMA1(x) (rotr32((x), 6) ^ rotr32((x), 11) ^ rotr32((x), 25))
#define SIGMA_a(x) (rotr32((x), 7)  ^ rotr32((x), 18) ^ ((x) >> 3))
#define SIGMA_b(x) (rotr32((x), 17) ^ rotr32((x), 19) ^ ((x) >> 10))


static const uint32_t k[] PROGMEM = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};


/*************************************************************************/

/**
 * block must be 512 Bit = 64 Byte long !!!
 */
void sha256_nextBlock (sha256_ctx_t *state, const void *block){
	uint32_t w[16];	/* this is 64 Byte large, */
	uint8_t  i;
	uint32_t a[8], t1, t2;

    /* init working variables */
    memcpy((void*)a,(void*)(state->h), 8 * 4);

    /* init w */
#if defined LITTLE_ENDIAN
    for (i = 0; i < 16; ++i) {
        w[i] = change_endian32(((uint32_t*)block)[i]);
    }
#elif defined BIG_ENDIAN
		memcpy((void*)w, block, 64);
#endif
/*
    for (i = 16; i < 64; ++i) {
        w[i] = SIGMA_b(w[i - 2]) + w[i - 7] + SIGMA_a(w[i - 15]) + w[i - 16];
    }
*/
	/* do the, fun stuff, */
    for (i=0; i<64; ++i) {
        if (i > 15) {
            w[i % 16] =   SIGMA_b(w[(i + 14) % 16])
                        + w[(i + 9) % 16]
                        + SIGMA_a(w[(i + 1) % 16])
                        + w[i % 16];
        }
        t1 = a[7] + SIGMA1(a[4]) + CH(a[4], a[5], a[6]) + pgm_read_dword(&k[i]) + w[i % 16];
        t2 = SIGMA0(a[0]) + MAJ(a[0], a[1], a[2]);
        memmove(&(a[1]), &(a[0]), 7 * 4); /* a[7]=a[6]; a[6]=a[5]; a[5]=a[4]; a[4]=a[3]; a[3]=a[2]; a[2]=a[1]; a[1]=a[0]; */
        a[4] += t1;
        a[0] = t1 + t2;
    }

	/* update, the, state, */
    for (i = 0; i < 8; ++i){
        state->h[i] += a[i];
    }
    state->length += 1;
}


/*************************************************************************/

/**
 * \brief function to process the last block being hashed
 * @param state Pointer to the context in which this block should be processed.
 * @param block Pointer to the message wich should be hashed.
 * @param length is the length of only THIS block in BITS not in bytes!
 *  bits are big endian, meaning high bits come first.
 * 	if you have a message with bits at the end, the byte must be padded with zeros
 */
void sha256_lastBlock(sha256_ctx_t *state, const void *block, uint16_t length){
	uint8_t lb[SHA256_BLOCK_BITS / 8]; /* local block */
	uint64_t msg_len;
	while(length>=SHA256_BLOCK_BITS){
		sha256_nextBlock(state, block);
		length -= SHA256_BLOCK_BITS;
		block = (uint8_t*)block + SHA256_BLOCK_BYTES;
	}

	msg_len = state->length;
	msg_len *= 512;
	msg_len += length;
	memcpy (&(lb[0]), block, length / 8);

	/* set the final one bit */
	if (length & 7){ // if we have single bits at the end
		lb[length / 8] = ((uint8_t*)(block))[length / 8];
	} else {
		lb[length / 8] = 0;
	}
	lb[length / 8] |= 0x80 >> (length & 7);
	length = (length / 8) + 1; /* from now on length contains the number of BYTES in lb*/
	/* pad with zeros */
	if (length > 64 - 8){ /* not enouth space for 64bit length value */
		memset((void*)(&(lb[length])), 0, 64 - length);
		sha256_nextBlock(state, lb);
		length = 0;
	}
	memset((void*)(&(lb[length])), 0, 56 - length);
	/* store the 64bit length value */
#if defined LITTLE_ENDIAN
    /* this is now rolled up */
	uint8_t i = 7;
	do {
	    lb[56 + i] = msg_len & 0xff;
	    msg_len >>= 8;
	} while (i--);
#elif defined BIG_ENDIAN
	*((uint64_t)&(lb[56])) = state->length;
#endif
	sha256_nextBlock(state, lb);
}


/*************************************************************************/

/*
 * length in bits!
 */
void sha256(sha256_hash_t *dest, const void *msg, uint32_t length){ /* length could be choosen longer but this is for µC */
	sha256_ctx_t s;
	sha256_init(&s);
	while(length >= SHA256_BLOCK_BITS){
		sha256_nextBlock(&s, msg);
		msg = (uint8_t*)msg + SHA256_BLOCK_BITS/8;
		length -= SHA256_BLOCK_BITS;
	}
	sha256_lastBlock(&s, msg, length);
	sha256_ctx2hash(dest,&s);
}



/*************************************************************************/

void sha256_ctx2hash(sha256_hash_t *dest, const sha256_ctx_t *state){
#if defined LITTLE_ENDIAN
	uint8_t i;
	for(i = 0; i < 8; ++i){
		((uint32_t*)dest)[i] = change_endian32(state->h[i]);
	}
#elif BIG_ENDIAN
	if (dest != state->h)
		memcpy(dest, state->h, SHA256_HASH_BITS/8);
#else
# error unsupported endian type!
#endif
}


