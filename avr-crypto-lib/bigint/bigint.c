/* bigint.c */
/*
    This file is part of the ARM-Crypto-Lib.
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
 * \file		bigint.c
 * \author		Daniel Otte
 * \date		2010-02-22
 * 
 * \license	    GPLv3 or later
 * 
 */
 

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define STR_LINE STRING(__LINE__)

#include "bigint.h"
#include <string.h>
#include <stdio.h>

#define PREFERE_HEAP_SPACE 1

#if PREFERE_HEAP_SPACE
#include <stdlib.h>

#define ALLOC_BIGINT_WORDS(var,words)                                      \
    bigint_word_t *(var) = malloc((words) * sizeof(bigint_word_t));        \
    if (!(var)) {                                                          \
        puts_P(PSTR("\n\nDBG: OOM ERROR (in arithmeics)!\n"));             \
        uart0_flush();                                                     \
        for(;;)                                                            \
            ;                                                              \
    }

#define FREE(x) free(x)

#else

#define ALLOC_BIGINT_WORDS(var,words) bigint_word_t var[words]
#define FREE(x)

#endif

#define DEBUG 1

#if DEBUG
#include "cli.h"
#include "uart.h"
#include "bigint_io.h"
#endif

#ifndef MAX
 #define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
 #define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define SET_FBS(a, v) do {(a)->info &= ~BIGINT_FBS_MASK; (a)->info |= (v);} while(0)
#define GET_FBS(a)   ((a)->info & BIGINT_FBS_MASK)
#define SET_NEG(a)   (a)->info |= BIGINT_NEG_MASK
#define SET_POS(a)   (a)->info &= ~BIGINT_NEG_MASK
#define XCHG(a,b)    do{(a) ^= (b); (b) ^= (a); (a) ^= (b);} while(0)
#define XCHG_PTR(a,b)    do{ a = (void*)(((intptr_t)(a)) ^ ((intptr_t)(b))); \
	                         b = (void*)(((intptr_t)(a)) ^ ((intptr_t)(b))); \
	                         a = (void*)(((intptr_t)(a)) ^ ((intptr_t)(b)));} while(0)

#define GET_SIGN(a) ((a)->info & BIGINT_NEG_MASK)

/******************************************************************************/
void bigint_adjust(bigint_t *a){
	while (a->length_W != 0 && a->wordv[a->length_W - 1] == 0) {
		a->length_W--;
	}
	if (a->length_W == 0) {
		a->info=0;
		return;
	}
	bigint_word_t t;
	uint8_t i = BIGINT_WORD_SIZE - 1;
	t = a->wordv[a->length_W - 1];
	while ((t & (((bigint_word_t)1) << (BIGINT_WORD_SIZE - 1))) == 0 && i) {
		t <<= 1;
		i--;
	}
	SET_FBS(a, i);
}

/******************************************************************************/

bigint_length_t bigint_length_b(const bigint_t *a){
	if(!a->length_W || a->length_W==0){
		return 0;
	}
	return (a->length_W-1) * BIGINT_WORD_SIZE + GET_FBS(a);
}

/******************************************************************************/

bigint_length_t bigint_length_B(const bigint_t *a){
	return a->length_W * sizeof(bigint_word_t);
}

/******************************************************************************/

int32_t bigint_get_first_set_bit(const bigint_t *a){
	if(a->length_W == 0) {
		return -1;
	}
	return (a->length_W-1) * sizeof(bigint_word_t) * CHAR_BIT + GET_FBS(a);
}


/******************************************************************************/

int32_t bigint_get_last_set_bit(const bigint_t *a){
	uint32_t r = 0;
	uint8_t b = 0;
	bigint_word_t x = 1;
	if (a->length_W == 0) {
		return -1;
	}
	while (a->wordv[r] == 0 && r < a->length_W) {
		++r;
	}
	if (a->wordv[r] == 0) {
		return (uint32_t)(-1);
	}
	while ((x&a->wordv[r])==0) {
		++b;
		x <<= 1;
	}
	return r * BIGINT_WORD_SIZE + b;
}

/******************************************************************************/

void bigint_copy(bigint_t *dest, const bigint_t *src){
    if(dest->wordv != src->wordv){
	    memcpy(dest->wordv, src->wordv, src->length_W * sizeof(bigint_word_t));
    }
    dest->length_W = src->length_W;
	dest->info = src->info;
}

/******************************************************************************/

/* this should be implemented in assembly */
void bigint_add_u(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	bigint_length_t i;
	bigint_wordplus_t t = 0LL;
	if (a->length_W < b->length_W) {
		XCHG_PTR(a, b);
	}
	for(i = 0; i < b->length_W; ++i) {
		t += a->wordv[i];
		t += b->wordv[i];
		dest->wordv[i] = (bigint_word_t)t;
		t >>= BIGINT_WORD_SIZE;
	}
	for(; i < a->length_W; ++i){
		t += a->wordv[i];
		dest->wordv[i] = (bigint_word_t)t;
		t >>= BIGINT_WORD_SIZE;
	}
	if(t){
		dest->wordv[i++] = (bigint_word_t)t;
	}
	dest->length_W = i;
	SET_POS(dest);
	bigint_adjust(dest);
}

/******************************************************************************/

/* this should be implemented in assembly */
void bigint_add_scale_u(bigint_t *dest, const bigint_t *a, bigint_length_t scale){
	if(a->length_W == 0){
		return;
	}
	if(scale == 0){
		bigint_add_u(dest, dest, a);
		return;
	}
	bigint_t x;
#if BIGINT_WORD_SIZE == 8
	memset(dest->wordv + dest->length_W, 0, MAX(dest->length_W, a->length_W + scale) - dest->length_W);
	x.wordv = dest->wordv + scale;
	x.length_W = dest->length_W - scale;
	if((int16_t)x.length_W < 0){
		x.length_W = 0;
		x.info = 0;
	} else {
		x.info = dest->info;
	}
	bigint_add_u(&x, &x, a);
	dest->length_W = x.length_W + scale;
	dest->info = 0;
	bigint_adjust(dest);
#else
	bigint_t s;
	bigint_length_t word_shift = scale / sizeof(bigint_word_t), byte_shift = scale % sizeof(bigint_word_t);
	bigint_word_t bv[a->length_W + 1];
	s.wordv = bv;
	bv[0] = bv[a->length_W] = 0;
	memcpy((uint8_t*)bv + byte_shift, a->wordv, a->length_W * sizeof(bigint_word_t));
	s.length_W = a->length_W + 1;
	bigint_adjust(&s);
	memset(dest->wordv + dest->length_W, 0, (MAX(dest->length_W, s.length_W + word_shift) - dest->length_W) * sizeof(bigint_word_t));
	x.wordv = dest->wordv + word_shift;
	x.length_W = dest->length_W - word_shift;
	if((int16_t)x.length_W < 0){
		x.length_W = 0;
		x.info = 0;
	}else{
		x.info = dest->info;
	}
	bigint_add_u(&x, &x, &s);
	dest->length_W = x.length_W + word_shift;
	dest->info = 0;
	bigint_adjust(dest);
#endif
}

/******************************************************************************/

/* this should be implemented in assembly */
void bigint_sub_u(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	int8_t borrow = 0;
	int8_t  r;
	bigint_wordplus_signed_t t = 0;
	bigint_length_t i;
	if(b->length_W == 0){
		bigint_copy(dest, a);
		SET_POS(dest);
		return;
	}
	if(a->length_W == 0){
		bigint_copy(dest, b);
		SET_NEG(dest);
		return;
	}
    r = bigint_cmp_u(a,b);
    if(r == 0){
        bigint_set_zero(dest);
        return;
    }
	if(r < 0){
		bigint_sub_u(dest, b, a);
		SET_NEG(dest);
		return;
	}
	for(i = 0; i < a->length_W; ++i){
		t = a->wordv[i];
		if(i < b->length_W){
			t -= b->wordv[i];
		}
		t -= borrow;
		dest->wordv[i] = (bigint_word_t)t;
		borrow = t < 0 ? 1 : 0;
	}
	SET_POS(dest);
	dest->length_W = i;
	bigint_adjust(dest);
}

/******************************************************************************/

int8_t bigint_cmp_u(const bigint_t *a, const bigint_t *b){
	if(a->length_W > b->length_W){
		return 1;
	}
	if(a->length_W < b->length_W){
		return -1;
	}
	if(a->length_W == 0){
		return 0;
	}
	bigint_length_t i;
	i = a->length_W - 1;
	do{
		if(a->wordv[i] != b->wordv[i]){
			if(a->wordv[i] > b->wordv[i]){
				return 1;
			}else{
				return -1;
			}
		}
	}while(i--);
	return 0;
}

/******************************************************************************/

void bigint_add_s(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	uint8_t s;
	s  = GET_SIGN(a)?2:0;
	s |= GET_SIGN(b)?1:0;
	switch(s){
		case 0: /* both positive */
			bigint_add_u(dest, a,b);
			SET_POS(dest);
			break;
		case 1: /* a positive, b negative */
			bigint_sub_u(dest, a, b);
			break;
		case 2: /* a negative, b positive */
			bigint_sub_u(dest, b, a);
			break;
		case 3: /* both negative */
			bigint_add_u(dest, a, b);
			SET_NEG(dest);
			break;
		default: /* how can this happen?*/
			break;
	}
}

/******************************************************************************/

void bigint_sub_s(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	uint8_t s;
	s  = GET_SIGN(a)?2:0;
	s |= GET_SIGN(b)?1:0;
	switch(s){
		case 0: /* both positive */
			bigint_sub_u(dest, a,b);
			break;
		case 1: /* a positive, b negative */
			bigint_add_u(dest, a, b);
			SET_POS(dest);
			break;
		case 2: /* a negative, b positive */
			bigint_add_u(dest, a, b);
			SET_NEG(dest);
			break;
		case 3: /* both negative */
			bigint_sub_u(dest, b, a);
			break;
		default: /* how can this happen?*/
					break;
	}

}

/******************************************************************************/

int8_t bigint_cmp_s(const bigint_t *a, const bigint_t *b){
	uint8_t s;
	if(a->length_W==0 && b->length_W==0){
		return 0;
	}
	s  = GET_SIGN(a)?2:0;
	s |= GET_SIGN(b)?1:0;
	switch(s){
		case 0: /* both positive */
			return bigint_cmp_u(a, b);
			break;
		case 1: /* a positive, b negative */
			return 1;
			break;
		case 2: /* a negative, b positive */
			return -1;
			break;
		case 3: /* both negative */
			return bigint_cmp_u(b, a);
			break;
		default: /* how can this happen?*/
					break;
	}
	return 0; /* just to satisfy the compiler */
}

/******************************************************************************/

void bigint_shiftleft_bits(bigint_t *a, uint8_t shift) {
    bigint_length_t i;
    bigint_wordplus_t t = 0;

    for (i = 0; i < a->length_W; ++i) {
        t |= ((bigint_wordplus_t)a->wordv[i]) << shift;
        a->wordv[i] = (bigint_word_t)t;
        t >>= BIGINT_WORD_SIZE;
    }
    if (t) {
        a->wordv[a->length_W++] = (bigint_word_t)t;
    }
    bigint_adjust(a);
}

/******************************************************************************/

void bigint_shiftleft(bigint_t *a, bigint_length_t shift){
	bigint_length_t byteshift;
	uint8_t bitshift;

	if (a->length_W == 0 || shift == 0) {
		return;
	}
	byteshift = shift / 8;
	bitshift = shift & 7;
    if (byteshift % sizeof(bigint_word_t)) {
        a->wordv[a->length_W + byteshift / sizeof(bigint_t)] = 0;
    }
	if (byteshift) {
		memmove(((uint8_t*)a->wordv) + byteshift, a->wordv, a->length_W * sizeof(bigint_word_t));
		memset(a->wordv, 0, byteshift);
        a->length_W += (byteshift + sizeof(bigint_word_t) - 1) / sizeof(bigint_word_t);
	}

	if (bitshift == 0) {
	    bigint_adjust(a);
	} else {
	    bigint_shiftleft_bits(a, bitshift);
	}
}

/******************************************************************************/

void bigint_shiftright_bits(bigint_t *a, uint8_t shift){
    bigint_length_t i;
    bigint_wordplus_t t = 0;

    i = a->length_W;
    while (i--) {
        t |= ((bigint_wordplus_t)(a->wordv[i])) << (BIGINT_WORD_SIZE - shift);
        a->wordv[i] = (bigint_word_t)(t >> BIGINT_WORD_SIZE);
        t <<= BIGINT_WORD_SIZE;
    }

    bigint_adjust(a);
}

/******************************************************************************/

void bigint_shiftright_1bit(bigint_t *a){
    bigint_length_t i;
    bigint_word_t t1 = 0, t2;

    i = a->length_W;
    while (i--) {
        t2 = a->wordv[i] & 1;
        a->wordv[i] >>= 1;
        a->wordv[i] |= t1;
        t1 = t2 << (BIGINT_WORD_SIZE - 1);
    }
    bigint_adjust(a);
}

/******************************************************************************/

void bigint_shiftright_1word(bigint_t *a){
    if (a->length_W == 0) {
        return;
    }
    memmove(a->wordv, &a->wordv[1], (a->length_W - 1) * sizeof(bigint_word_t));
    a->length_W--;
}

/******************************************************************************/

void bigint_shiftright(bigint_t *a, bigint_length_t shift){
	bigint_length_t byteshift = shift / 8;
	uint8_t bitshift = shift & 7;

	if (a->length_W == 0) {
	    return;
	}

	if(bigint_get_first_set_bit(a) < shift){ /* we would shift out more than we have */
		bigint_set_zero(a);
		return;
	}

	if(byteshift){
		memmove(a->wordv, (uint8_t*)a->wordv + byteshift, a->length_W * sizeof(bigint_word_t) - byteshift);
		memset((uint8_t*)&a->wordv[a->length_W] - byteshift, 0, byteshift);
	    a->length_W -= byteshift / sizeof(bigint_word_t);
	    bigint_adjust(a);
	}

    if(bitshift != 0){
        bigint_shiftright_bits(a, bitshift);
	}
}

/******************************************************************************/

void bigint_xor(bigint_t *dest, const bigint_t *a){
	bigint_length_t i;
	for(i=0; i<a->length_W; ++i){
		dest->wordv[i] ^= a->wordv[i];
	}
	bigint_adjust(dest);
}

/******************************************************************************/

void bigint_set_zero(bigint_t *a){
	a->length_W = 0;
}

/******************************************************************************/

/* using the Karatsuba-Algorithm */
/* x*y = (xh*yh)*b**2n + ((xh+xl)*(yh+yl) - xh*yh - xl*yl)*b**n + yh*yl */
void bigint_mul_u(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	if (a->length_W == 0 || b->length_W == 0) {
		bigint_set_zero(dest);
		return;
	}
	if (dest == a || dest == b) {
		bigint_t d;
		bigint_word_t d_b[a->length_W + b->length_W];
		d.wordv = d_b;
		bigint_mul_u(&d, a, b);
		bigint_copy(dest, &d);
		return;
	}
	if (a->length_W == 1 || b->length_W == 1) {
		if (a->length_W != 1) {
			XCHG_PTR(a,b);
		}
		bigint_wordplus_t t = 0;
		bigint_length_t i;
		bigint_word_t x = a->wordv[0];
		for (i = 0; i < b->length_W; ++i) {
			t += ((bigint_wordplus_t)b->wordv[i]) * ((bigint_wordplus_t)x);
			dest->wordv[i] = (bigint_word_t)t;
			t >>= BIGINT_WORD_SIZE;
		}
		dest->length_W = i;
		if (t) {
		    dest->wordv[i] = (bigint_word_t)t;
		    dest->length_W += 1;
		}
		dest->info = 0;
		bigint_adjust(dest);
		return;
	}
	if (a->length_W * sizeof(bigint_word_t) <= 4 && b->length_W * sizeof(bigint_word_t) <= 4) {
		uint32_t p = 0, q = 0;
		uint64_t r;
		memcpy(&p, a->wordv, a->length_W * sizeof(bigint_word_t));
		memcpy(&q, b->wordv, b->length_W * sizeof(bigint_word_t));
		r = (uint64_t)p * (uint64_t)q;
		memcpy(dest->wordv, &r, (dest->length_W = a->length_W + b->length_W) * sizeof(bigint_word_t));
		bigint_adjust(dest);
		return;
	}
	/* split a in xh & xl; split b in yh & yl */
	const bigint_length_t n = (MAX(a->length_W, b->length_W) + 1) / 2;
	bigint_t xl, xh, yl, yh;
	xl.wordv = a->wordv;
	yl.wordv = b->wordv;
	if (a->length_W <= n) {
		bigint_set_zero(&xh);
		xl.length_W = a->length_W;
		xl.info = a->info;
	} else {
		xl.length_W = n;
		xl.info = 0;
		bigint_adjust(&xl);
		xh.wordv = &(a->wordv[n]);
		xh.length_W = a->length_W-n;
		xh.info = a->info;
	}
	if (b->length_W <= n) {
		bigint_set_zero(&yh);
		yl.length_W = b->length_W;
		yl.info = b->info;
	} else {
		yl.length_W = n;
		yl.info = 0;
		bigint_adjust(&yl);
		yh.wordv = &(b->wordv[n]);
		yh.length_W = b->length_W-n;
		yh.info = b->info;
	}
	/* now we have split up a and b */
	/* remember we want to do:
	 * x*y = (xh * b**n + xl) * (yh * b**n + yl)
	 *     = (xh * yh) * b**2n + xh * b**n * yl + yh * b**n * xl + xl * yl
	 *     = (xh * yh) * b**2n + (xh * yl + yh * xl) * b**n + xl *yl
	 *     // xh * yl + yh * xl = (xh + yh) * (xl + yl) - xh * yh - xl * yl
	 * x*y = (xh * yh) * b**2n + ((xh+xl)*(yh+yl) - xh*yh - xl*yl)*b**n + xl*yl
	 *          5              9     2   4   3    7   5   6   1         8   1
	 */
	ALLOC_BIGINT_WORDS(tmp_w, 2 * n + 2);
	ALLOC_BIGINT_WORDS(m_w, 2 * n + 2);
	bigint_t tmp, tmp2, m;
	tmp.wordv = tmp_w;
	tmp2.wordv = &(tmp_w[n + 1]);
	m.wordv = m_w;

	bigint_mul_u(dest, &xl, &yl);  /* 1: dest <= xl*yl     */
	bigint_add_u(&tmp2, &xh, &xl); /* 2: tmp2 <= xh+xl     */
	bigint_add_u(&tmp, &yh, &yl);  /* 3: tmp  <= yh+yl     */
	bigint_mul_u(&m, &tmp2, &tmp); /* 4: m    <= tmp2*tmp  */
	bigint_mul_u(&tmp, &xh, &yh);  /* 5: tmp  <= xh*yh     */
	bigint_sub_u(&m, &m, dest);    /* 6: m    <= m-dest    */
    bigint_sub_u(&m, &m, &tmp);    /* 7: m    <= m-tmp     */
	bigint_add_scale_u(dest, &m, n * sizeof(bigint_word_t));       /* 8: dest <= dest+m**n*/
	bigint_add_scale_u(dest, &tmp, 2 * n * sizeof(bigint_word_t)); /* 9: dest <= dest+tmp**(2*n) */
	FREE(m_w);
	FREE(tmp_w);
}

/******************************************************************************/

void bigint_mul_s(bigint_t *dest, const bigint_t *a, const bigint_t *b){
	uint8_t s;
	s  = GET_SIGN(a)?2:0;
	s |= GET_SIGN(b)?1:0;
	switch(s){
		case 0: /* both positive */
			bigint_mul_u(dest, a,b);
			SET_POS(dest);
			break;
		case 1: /* a positive, b negative */
			bigint_mul_u(dest, a,b);
			SET_NEG(dest);
			break;
		case 2: /* a negative, b positive */
			bigint_mul_u(dest, a,b);
			SET_NEG(dest);
			break;
		case 3: /* both negative */
			bigint_mul_u(dest, a,b);
			SET_POS(dest);
			break;
		default: /* how can this happen?*/
			break;
	}
}

/******************************************************************************/

void bigint_square(bigint_t *dest, const bigint_t *a) {
    union __attribute__((packed)) {
        bigint_word_t u[2];
        bigint_wordplus_t uv;
    } acc;
    bigint_word_t q, c1, c2;
    bigint_length_t i, j;

    if (a->length_W * sizeof(bigint_word_t) <= 4) {
        uint64_t r = 0;
        memcpy(&r, a->wordv, a->length_W * sizeof(bigint_word_t));
        r = r * r;
        memcpy(dest->wordv, &r, 2 * a->length_W * sizeof(bigint_word_t));
        SET_POS(dest);
        dest->length_W = 2 * a->length_W;
        bigint_adjust(dest);
        return;
    }

    if (dest->wordv == a->wordv) {
        bigint_t d;
        ALLOC_BIGINT_WORDS(d_w, a->length_W * 2);
        d.wordv = d_w;
        bigint_square(&d, a);
        bigint_copy(dest, &d);
        FREE(d_w);
        return;
    }

    memset(dest->wordv, 0, a->length_W * 2 * sizeof(bigint_word_t));

    for (i = 0; i < a->length_W; ++i) {
        acc.uv = (bigint_wordplus_t)a->wordv[i] * (bigint_wordplus_t)a->wordv[i] + (bigint_wordplus_t)dest->wordv[2 * i];
        dest->wordv[2 * i] = acc.u[0];
        c1 = acc.u[1];
        c2 = 0;
        for (j = i + 1; j < a->length_W; ++j) {
            acc.uv = (bigint_wordplus_t)a->wordv[i] * (bigint_wordplus_t)a->wordv[j];
            q = acc.u[1] >> (BIGINT_WORD_SIZE - 1);
            acc.uv <<= 1;
            acc.uv += dest->wordv[i + j];
            q += (acc.uv < dest->wordv[i + j]);
            acc.uv += c1;
            q += (acc.uv < c1);
            dest->wordv[i + j] = acc.u[0];
            c1 = (bigint_wordplus_t)acc.u[1] + c2;
            c2 = q + (c1 < c2);
        }
        dest->wordv[i + a->length_W] += c1;
        if (i < a->length_W - 1) {
            dest->wordv[i + a->length_W + 1] = c2 + (dest->wordv[i + a->length_W] < c1);
        }
    }
    dest->info = 0;
    dest->length_W = 2 * a->length_W;
    bigint_adjust(dest);
}

/******************************************************************************/

void bigint_sub_u_bitscale(bigint_t *a, const bigint_t *b, bigint_length_t bitscale){
	bigint_t tmp, x;
	const bigint_length_t word_shift = bitscale / BIGINT_WORD_SIZE;

	if (a->length_W < b->length_W + word_shift) {
#if DEBUG
		cli_putstr("\r\nDBG: *bang*\r\n");
#endif
		bigint_set_zero(a);
		return;
	}
    ALLOC_BIGINT_WORDS(tmp_w, b->length_W + 1);
	tmp.wordv = tmp_w;
	bigint_copy(&tmp, b);
	bigint_shiftleft_bits(&tmp, bitscale % BIGINT_WORD_SIZE);

	x.info = a->info;
	x.wordv = &(a->wordv[word_shift]);
	x.length_W = a->length_W - word_shift;

	bigint_sub_u(&x, &x, &tmp);
	FREE(tmp_w);
	bigint_adjust(a);
	return;
}

/******************************************************************************/

void bigint_reduce(bigint_t *a, const bigint_t *r){
	uint8_t rfbs = GET_FBS(r);
	if (r->length_W == 0 || a->length_W == 0) {
		return;
	}

	if (bigint_length_b(a) + 3 > bigint_length_b(r)) {
        if ((r->length_W * sizeof(bigint_word_t) <= 4) && (a->length_W * sizeof(bigint_word_t) <= 4)) {
            uint32_t p = 0, q = 0;
            memcpy(&p, a->wordv, a->length_W * sizeof(bigint_word_t));
            memcpy(&q, r->wordv, r->length_W * sizeof(bigint_word_t));
            p %= q;
            memcpy(a->wordv, &p, a->length_W * sizeof(bigint_word_t));
            a->length_W = r->length_W;
            bigint_adjust(a);
            return;
        }
        unsigned shift;
        while (a->length_W > r->length_W) {
            shift = (a->length_W - r->length_W) * CHAR_BIT * sizeof(bigint_word_t) + GET_FBS(a) - rfbs - 1;
            bigint_sub_u_bitscale(a, r, shift);
        }
        while ((GET_FBS(a) > rfbs) && (a->length_W == r->length_W)) {
            shift = GET_FBS(a) - rfbs - 1;
            bigint_sub_u_bitscale(a, r, shift);
        }
	}
	while (bigint_cmp_u(a, r) >= 0) {
		bigint_sub_u(a, a, r);
	}
	bigint_adjust(a);
}

/******************************************************************************/

/* calculate dest = a**exp % r */
/* using square&multiply */
void bigint_expmod_u_sam(bigint_t *dest, const bigint_t *a, const bigint_t *exp, const bigint_t *r){
	if (a->length_W == 0) {
	    bigint_set_zero(dest);
		return;
	}

    if(exp->length_W == 0){
        dest->info = 0;
        dest->length_W = 1;
        dest->wordv[0] = 1;
        return;
    }

    bigint_t res, base;
    bigint_word_t t;
	bigint_length_t i;
	uint8_t j;

	ALLOC_BIGINT_WORDS(base_w, MAX(a->length_W, r->length_W));
	ALLOC_BIGINT_WORDS(res_w, r->length_W * 2);

	res.wordv = res_w;
	base.wordv = base_w;
	bigint_copy(&base, a);
	bigint_reduce(&base, r);
	res.wordv[0] = 1;
	res.length_W = 1;
	res.info = 0;
	bigint_adjust(&res);
	bigint_copy(&res, &base);
	uint8_t flag = 0;
	t = exp->wordv[exp->length_W - 1];
	for (i = exp->length_W; i > 0; --i) {
		t = exp->wordv[i - 1];
		for (j = BIGINT_WORD_SIZE; j > 0; --j) {
			if (!flag) {
				if (t & (((bigint_word_t)1) << (BIGINT_WORD_SIZE - 1))) {
					flag = 1;
				}
			} else {
				bigint_square(&res, &res);
				bigint_reduce(&res, r);
				if(t & (((bigint_word_t)1) << (BIGINT_WORD_SIZE - 1))){
					bigint_mul_u(&res, &res, &base);
					bigint_reduce(&res, r);
				}
			}
			t <<= 1;
		}
	}

	SET_POS(&res);
	bigint_copy(dest, &res);
	FREE(res_w);
	FREE(base_w);
}

/******************************************************************************/
/* gcd <-- gcd(x,y) a*x+b*y=gcd */
void bigint_gcdext(bigint_t *gcd, bigint_t *a, bigint_t *b, const bigint_t *x, const bigint_t *y){
	 bigint_length_t i = 0;
	 if(x->length_W == 0 || y->length_W == 0){
	     if(gcd){
	         bigint_set_zero(gcd);
	     }
	     if(a){
	         bigint_set_zero(a);
	     }
         if(b){
             bigint_set_zero(b);
         }
		 return;
	 }
	 if(x->length_W == 1 && x->wordv[0] == 1){
	     if(gcd){
             gcd->length_W = 1;
             gcd->wordv[0] = 1;
             gcd->info = 0;
	     }
		 if(a){
			 a->length_W = 1;
			 a->wordv[0] = 1;
			 SET_POS(a);
			 bigint_adjust(a);
		 }
		 if(b){
			 bigint_set_zero(b);
		 }
		 return;
	 }
	 if(y->length_W == 1 && y->wordv[0] == 1){
		 if(gcd){
             gcd->length_W = 1;
             gcd->wordv[0] = 1;
             gcd->info = 0;
		 }
		 if(b){
			 b->length_W = 1;
			 b->wordv[0] = 1;
			 SET_POS(b);
			 bigint_adjust(b);
		 }
		 if(a){
			 bigint_set_zero(a);
		 }
		 return;
	 }

	 while(x->wordv[i] == 0 && y->wordv[i] == 0){
		 ++i;
	 }

	 ALLOC_BIGINT_WORDS(g_w, i + 2);
	 ALLOC_BIGINT_WORDS(x_w, x->length_W - i);
	 ALLOC_BIGINT_WORDS(y_w, y->length_W - i);
	 ALLOC_BIGINT_WORDS(u_w, x->length_W - i);
     ALLOC_BIGINT_WORDS(v_w, y->length_W - i);
     ALLOC_BIGINT_WORDS(a_w, y->length_W + 2);
     ALLOC_BIGINT_WORDS(c_w, y->length_W + 2);
     ALLOC_BIGINT_WORDS(b_w, x->length_W + 2);
     ALLOC_BIGINT_WORDS(d_w, x->length_W + 2);

	 bigint_t g, x_, y_, u, v, a_, b_, c_, d_;

	 g.wordv = g_w;
	 x_.wordv = x_w;
	 y_.wordv = y_w;
	 memset(g_w, 0, i * sizeof(bigint_word_t));
	 g_w[i] = 1;
	 g.length_W = i + 1;
	 g.info = 0;
	 x_.info = y_.info = 0;
	 x_.length_W = x->length_W - i;
	 y_.length_W = y->length_W - i;
	 memcpy(x_.wordv, x->wordv + i, x_.length_W * sizeof(bigint_word_t));
	 memcpy(y_.wordv, y->wordv + i, y_.length_W * sizeof(bigint_word_t));

	 for(i = 0; (x_.wordv[0] & ((bigint_word_t)1 << i)) == 0 && (y_.wordv[0] & ((bigint_word_t)1 << i)) == 0; ++i)
	     ;

	 bigint_adjust(&x_);
	 bigint_adjust(&y_);

	 if(i){
		 bigint_shiftleft_bits(&g, i);
		 bigint_shiftright_bits(&x_, i);
		 bigint_shiftright_bits(&y_, i);
	 }

	 u.wordv = u_w;
	 v.wordv = v_w;
	 a_.wordv = a_w;
	 b_.wordv = b_w;
	 c_.wordv = c_w;
	 d_.wordv = d_w;

	 bigint_copy(&u, &x_);
	 bigint_copy(&v, &y_);
	 a_.wordv[0] = 1;
	 a_.length_W = 1;
	 a_.info = 0;
	 d_.wordv[0] = 1;
	 d_.length_W = 1;
	 d_.info = 0;
	 bigint_set_zero(&b_);
	 bigint_set_zero(&c_);
	 do {
		 while ((u.wordv[0] & 1) == 0) {
			 bigint_shiftright_1bit(&u);
			 if((a_.wordv[0] & 1) || (b_.wordv[0] & 1)){
				 bigint_add_s(&a_, &a_, &y_);
				 bigint_sub_s(&b_, &b_, &x_);
			 }
			 bigint_shiftright_1bit(&a_);
			 bigint_shiftright_1bit(&b_);
		 }
		 while ((v.wordv[0] & 1) == 0) {
		     bigint_shiftright_1bit(&v);
			 if((c_.wordv[0] & 1) || (d_.wordv[0] & 1)){
				 bigint_add_s(&c_, &c_, &y_);
				 bigint_sub_s(&d_, &d_, &x_);
			 }
			 bigint_shiftright_1bit(&c_);
			 bigint_shiftright_1bit(&d_);
		 }
		 if(bigint_cmp_u(&u, &v) >= 0){
			bigint_sub_u(&u, &u, &v);
			bigint_sub_s(&a_, &a_, &c_);
			bigint_sub_s(&b_, &b_, &d_);
		 }else{
			bigint_sub_u(&v, &v, &u);
			bigint_sub_s(&c_, &c_, &a_);
			bigint_sub_s(&d_, &d_, &b_);
		 }
	 } while(u.length_W);
	 if(gcd){
		 bigint_mul_s(gcd, &v, &g);
	 }
	 if(a){
		bigint_copy(a, &c_);
	 }
	 if(b){
		 bigint_copy(b, &d_);
	 }

	 FREE(d_w);
	 FREE(b_w);
	 FREE(c_w);
	 FREE(a_w);
     FREE(v_w);
     FREE(u_w);
     FREE(y_w);
     FREE(x_w);
     FREE(g_w);
}

/******************************************************************************/

void bigint_inverse(bigint_t *dest, const bigint_t *a, const bigint_t *m){
	bigint_gcdext(NULL, dest, NULL, a, m);
	while(dest->info&BIGINT_NEG_MASK){
		bigint_add_s(dest, dest, m);
	}
}

/******************************************************************************/

void bigint_changeendianess(bigint_t *a){
	uint8_t t, *p, *q;
	p = (uint8_t*)a->wordv;
	q = p + a->length_W * sizeof(bigint_word_t) - 1;
	while(p < q){
		t = *p;
		*p = *q;
		*q = t;
		++p; --q;
	}
}

/******************************************************************************/

void bigint_mul_word_u(bigint_t *a, bigint_word_t b){
    bigint_wordplus_t c0 = 0, c1 = 0;
    bigint_length_t i;

    if(b == 0){
        bigint_set_zero(a);
        return;
    }

    for(i = 0; i < a->length_W; ++i){
        c1 = ((bigint_wordplus_t)(a->wordv[i])) * ((bigint_wordplus_t)b);
        c1 += c0;
        a->wordv[i] = (bigint_word_t)c1;
        c0 = c1 >> BIGINT_WORD_SIZE;
    }
    if(c0){
        a->wordv[a->length_W] = (bigint_word_t)c0;
        a->length_W += 1;
    }
    bigint_adjust(a);
}

/******************************************************************************/

void bigint_clip(bigint_t *dest, bigint_length_t length_W){
    if(dest->length_W > length_W){
        dest->length_W = length_W;
    }
    bigint_adjust(dest);
}

/******************************************************************************/
/*
 * m_ = m * m'[0]
 * dest = (a * b) % m (?)
 */

void bigint_mont_mul(bigint_t *dest, const bigint_t *a, const bigint_t *b, const bigint_t *m, const bigint_t *m_){
    const bigint_length_t s = MAX(MAX(a->length_W, b->length_W), m->length_W);
    bigint_t u, t;

    bigint_length_t i;

    if (a->length_W == 0 || b->length_W == 0) {
        bigint_set_zero(dest);
        return;
    }
    ALLOC_BIGINT_WORDS(u_w, s + 2);
    ALLOC_BIGINT_WORDS(t_w, s + 2);
    u.wordv = u_w;
    u.info = 0;
    u.length_W = 0;
    t.wordv = t_w;
    for (i = 0; i < a->length_W; ++i) {
        bigint_copy(&t, b);
        bigint_mul_word_u(&t, a->wordv[i]);
        bigint_add_u(&u, &u, &t);
        bigint_copy(&t, m_);
        if (u.length_W != 0) {
            bigint_mul_word_u(&t, u.wordv[0]);
            bigint_add_u(&u, &u, &t);
        }
        bigint_shiftright_1word(&u);
    }
    for (; i < s; ++i) {
        bigint_copy(&t, m_);
        if (u.length_W != 0) {
            bigint_mul_word_u(&t, u.wordv[0]);
            bigint_add_u(&u, &u, &t);
        }
        bigint_shiftright_1word(&u);
    }
    bigint_reduce(&u, m);
    bigint_copy(dest, &u);
    FREE(t_w);
    FREE(u_w);
}

/******************************************************************************/

void bigint_mont_red(bigint_t *dest, const bigint_t *a, const bigint_t *m, const bigint_t *m_){
    bigint_t u, t;
    bigint_length_t i, s = MAX(a->length_W, MAX(m->length_W, m_->length_W));

    if (a->length_W == 0) {
        bigint_set_zero(dest);
        return;
    }

    ALLOC_BIGINT_WORDS(u_w, s + 2);
    ALLOC_BIGINT_WORDS(t_w, s + 2);
    t.wordv = t_w;
    u.wordv = u_w;
    bigint_copy(&u, a);
    for (i = 0; i < m->length_W; ++i) {
        bigint_copy(&t, m_);
        if (u.length_W != 0) {
            bigint_mul_word_u(&t, u.wordv[0]);
            bigint_add_u(&u, &u, &t);
        }
        bigint_shiftright_1word(&u);
    }
    bigint_reduce(&u, m);
    bigint_copy(dest, &u);
    FREE(t_w);
    FREE(u_w);
}

/******************************************************************************/
/*
 * m_ = m * (- m0^-1 (mod 2^W))
 */
void bigint_mont_gen_m_(bigint_t* dest, const bigint_t* m){
    bigint_word_t x_w[2], m_w_0[1];
    bigint_t x, m_0;
    if (m->length_W == 0) {
        bigint_set_zero(dest);
        return;
    }
    if ((m->wordv[0] & 1) == 0) {
#if DEBUG
        printf_P(PSTR("ERROR: m must not be even, m = "));
        bigint_print_hex(m);
        putchar('\n');
        uart0_flush();
#endif
        return;
    }
    x.wordv = x_w;
    x.info = 0;
    x.length_W = 2;
    x_w[0] = 0;
    x_w[1] = 1;
    m_0.wordv = m_w_0;
    m_0.info = 0;
    m_0.length_W = 1;
    m_0.wordv[0] = m->wordv[0];
    bigint_adjust(&x);
    bigint_adjust(&m_0);
    bigint_inverse(dest, &m_0, &x);
    bigint_sub_s(&x, &x, dest);
    bigint_copy(dest, m);
    bigint_mul_word_u(dest, x.wordv[0]);

}

/******************************************************************************/

/*
 * dest = a * R mod m
 */
void bigint_mont_trans(bigint_t *dest, const bigint_t *a, const bigint_t *m){
    bigint_t t;

    ALLOC_BIGINT_WORDS(t_w, a->length_W + m->length_W);
    t.wordv = t_w;
    memset(t_w, 0, m->length_W * sizeof(bigint_word_t));
    memcpy(&t_w[m->length_W], a->wordv, a->length_W * sizeof(bigint_word_t));
    t.info = a->info;
    t.length_W = a->length_W + m->length_W;
    bigint_reduce(&t, m);
    bigint_copy(dest, &t);
    FREE(t_w);
}

/******************************************************************************/

/* calculate dest = a**exp % r */
/* using square&multiply */
void bigint_expmod_u_mont_accel(bigint_t *dest, const bigint_t *a, const bigint_t *exp, const bigint_t *r, const bigint_t *m_){
    if(r->length_W == 0) {
        return;
    }

    bigint_t res, ax;
    bigint_word_t t;
    bigint_length_t i;
    uint8_t j;

    if (exp->length_W == 0) {
        dest->length_W = 1;
        dest->info = 0;
        dest->wordv[0] = 1;
        return;
    }

    ALLOC_BIGINT_WORDS(res_w, r->length_W * 2);
    ALLOC_BIGINT_WORDS(ax_w, MAX(r->length_W, a->length_W));

    res.wordv = res_w;
    ax.wordv = ax_w;

    res.wordv[0] = 1;
    res.length_W = 1;
    res.info = 0;

    bigint_copy(&ax, a);
    bigint_reduce(&ax, r);

    bigint_mont_trans(&ax, &ax, r);
    bigint_mont_trans(&res, &res, r);

    uint8_t flag = 0;
    t = exp->wordv[exp->length_W - 1];
    for (i = exp->length_W; i > 0; --i) {
        t = exp->wordv[i - 1];
        for(j = BIGINT_WORD_SIZE; j > 0; --j){
            if (!flag) {
                if(t & (((bigint_word_t)1) << (BIGINT_WORD_SIZE - 1))){
                    flag = 1;
                }
            }
            if (flag) {
                bigint_square(&res, &res);
                bigint_mont_red(&res, &res, r, m_);
                if (t & (((bigint_word_t)1) << (BIGINT_WORD_SIZE - 1))) {
                    bigint_mont_mul(&res, &res, &ax, r, m_);
                }
            }
            t <<= 1;
        }
    }
    SET_POS(&res);
    bigint_mont_red(dest, &res, r, m_);
    FREE(ax_w);
    FREE(res_w);
}

/******************************************************************************/

void bigint_expmod_u_mont_sam(bigint_t *dest, const bigint_t *a, const bigint_t *exp, const bigint_t *r){
    if(r->length_W == 0) {
        return;
    }
    if(a->length_W == 0) {
        bigint_set_zero(dest);
        return;
    }
    bigint_t m_;
    bigint_word_t m_w_[r->length_W + 1];
    m_.wordv = m_w_;
    bigint_mont_gen_m_(&m_, r);
    bigint_expmod_u_mont_accel(dest, a, exp, r,&m_);
}

/******************************************************************************/

void bigint_expmod_u(bigint_t *dest, const bigint_t *a, const bigint_t *exp, const bigint_t *r){
#if 0
    printf("\nDBG: expmod_u (a ** e %% m) <%s %s %d>\n\ta: ", __FILE__, __func__, __LINE__);
    bigint_print_hex(a);
    printf("\n\te: ");
    bigint_print_hex(exp);
    printf("\n\tm: ");
    bigint_print_hex(r);
#endif
    if (r->wordv[0] & 1) {
        bigint_expmod_u_mont_sam(dest, a, exp, r);
    } else {
        bigint_expmod_u_sam(dest, a, exp, r);
    }
}
















