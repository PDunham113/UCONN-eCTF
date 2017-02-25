/* bigint2.h */
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

#ifndef BIGINT2_H_
#define BIGINT2_H_

#include <stdint.h>
#include <limits.h>
#include <stdlib.h>

#define BIGINT_WORD_SIZE 8

#if BIGINT_WORD_SIZE == 8
typedef uint8_t  bigint_word_t;
typedef uint16_t bigint_wordplus_t;
typedef int16_t  bigint_wordplus_signed_t;
#elif BIGINT_WORD_SIZE == 16
typedef uint16_t  bigint_word_t;
typedef uint32_t bigint_wordplus_t;
typedef int32_t  bigint_wordplus_signed_t;
#elif BIGINT_WORD_SIZE == 32
typedef uint32_t  bigint_word_t;
typedef uint64_t bigint_wordplus_t;
typedef int64_t  bigint_wordplus_signed_t;
#else
#error "INVALID VALUE FOR BIGINT_WORD_SIZE"
#endif

typedef uint16_t bigint_length_t;
typedef uint8_t bigint_info_t;

#define BIGINT_SIGN_MASK 0x80
#define BIGINT_GET_SIGN(x) ((x)->info & BIGINT_SIGN_MASK)
#define BIGINT_SET_POS(x) ((x)->info &= ~BIGINT_SIGN_MASK)
#define BIGINT_SET_NEG(x) ((x)->info |= BIGINT_SIGN_MASK)


typedef struct {
    bigint_word_t *wordv;
    bigint_length_t length_W;
    bigint_length_t allocated_W;
    bigint_info_t info;
} bigint_t;

extern void *(*int_realloc)(void *ptr, size_t size);
extern void (*int_free)(void *ptr);

int bigint_copy(bigint_t *dest, const bigint_t *a);

int bigint_free(bigint_t *a);

/**
 * \brief dest = |a| + |b|
 * Adds the bigints a and b and stores the result in dest.
 * Signs are ignored.
 */
int bigint_add_u(bigint_t *dest, const bigint_t *a, const bigint_t *b);

/**
 * \brief dest = |a| - |b|
 * Subtracts b from a and stores the result in dest.
 * Signs are ignored
 */
int bigint_sub_u(bigint_t *dest, const bigint_t *a, const bigint_t *b);

/**
 * \brief a <<= 1
 */
int bigint_shiftleft_1bit(bigint_t *a);

/**
 * \brief dest = a << s
 */
int bigint_shiftleft(bigint_t *dest, const bigint_t *a, bigint_length_t s);

/**
 * \brief a >>= 1
 */
int bigint_shiftright_1bit(bigint_t *a);

/**
 * \brief dest = a ** 2
 */
int bigint_square(bigint_t *dest, const bigint_t *a);

/**
 * \brief dest = |a * b|
 * unsigned multiply a bigint (a) by a word (b)
 */
int bigint_mul_word(bigint_t *dest, const bigint_t *a, const bigint_word_t b);

/**
 * \brief dest = a * b
 */
int bigint_mul_schoolbook(bigint_t *dest, const bigint_t *a, const bigint_t *b);

/*
 * UNSAFE!!!
 * a <=> b
 */
int8_t bigint_cmp_u(const bigint_t *a, const bigint_t *b);

/*
 * UNSAFE!!!
 * a <=> b * (B ** s)
 */
int8_t bigint_cmp_scale(const bigint_t *a, const bigint_t *b, bigint_length_t s);

/*
 * UNSAFE!!!
 */
int bigint_adjust_length(bigint_t *a);

bigint_length_t bigint_get_first_set_bit(const bigint_t *a);

int bigint_divide(bigint_t *q, bigint_t *r, const bigint_t *a, const bigint_t *b);

int8_t bigint_cmp_s(const bigint_t *a, const bigint_t *b);

int bigint_gcdext(bigint_t *gcd, bigint_t *x, bigint_t *y, const bigint_t *a, const bigint_t *b);



#endif /* BIGINT2_H_ */
