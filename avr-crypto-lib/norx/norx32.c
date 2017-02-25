/* norx32.c */
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


#include <inttypes.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include <memxor.h>
#include <norx32.h>

#include <stdio.h>
#include "cli.h"

#define R0  8
#define R1 11
#define R2 16
#define R3 31

#define U0 0x243f6a88l
#define U1 0x85a308d3l
#define U2 0x13198a2el
#define U3 0x03707344l
#define U4 0x254f537al
#define U5 0x38531d48l
#define U6 0x839c6E83l
#define U7 0xf97a3ae5l
#define U8 0x8c91d88cl
#define U9 0x11eafb59l

#define WORD_SIZE 32

#define RATE_WORDS 10
#define CAPACITY_WORDS 6

#define RATE_BITS (RATE_WORDS * WORD_SIZE)
#define CAPACITY_BITS (CAPACITY_WORDS * WORD_SIZE)

#define RATE_BYTES (RATE_BITS / 8)
#define CAPACITY_BYTES (CAPACITY_BITS / 8)

#define TAG_HEADER    0x01
#define TAG_PAYLOAD   0x02
#define TAG_TRAILER   0x04
#define TAG_TAG       0x08
#define TAG_BRANCHING 0x10
#define TAG_MERGING   0x20


#define SET_TAG(ctx, t) do { \
        ((uint8_t*)&(ctx)->s[15])[0] ^= (t); \
    } while (0)

#define TOGGLE_BIT(buf, bit_addr) do { \
        ((uint8_t*)(buf))[(bit_addr) / 8] ^= (1 << ((bit_addr) & 7)); \
    } while (0)

#define TRUNCATE_BUFFER(buf, bits) do { \
        if (bits & 7) { \
            ((uint8_t*)(buf))[(bits) / 8] &= 0xff >> (7 - ((bits) & 7)); \
        } \
    } while (0)

#if 0
void norx32_dump(const norx32_ctx_t *ctx)
{
    printf("\n--- DUMP STATE ---");
    printf("\n\t%08lX %08lX %08lX %08lX", ctx->s[ 0], ctx->s[ 1], ctx->s[ 2], ctx->s[ 3]);
    printf("\n\t%08lX %08lX %08lX %08lX", ctx->s[ 4], ctx->s[ 5], ctx->s[ 6], ctx->s[ 7]);
    printf("\n\t%08lX %08lX %08lX %08lX", ctx->s[ 8], ctx->s[ 9], ctx->s[10], ctx->s[11]);
    printf("\n\t%08lX %08lX %08lX %08lX", ctx->s[12], ctx->s[13], ctx->s[14], ctx->s[15]);
    printf("\n--- END ---\n");
}
#endif

static void phi(uint32_t *a, uint32_t *b)
{
    *a = (*a ^ *b) ^ ((*a & *b) << 1);
}

static void xrot(uint32_t *a, uint32_t *b, uint8_t r)
{
    uint32_t x;
    x = *a ^ *b;
    *a = (x << (32 - r)) | (x >> r);
}

#define A (v[3])
#define B (v[2])
#define C (v[1])
#define D (v[0])

static const uint8_t g2_table[8][4] PROGMEM = {
        {0, 4,  8, 12},
        {1, 5,  9, 13},
        {2, 6, 10, 14},
        {3, 7, 11, 15},
        {0, 5, 10, 15},
        {1, 6, 11, 12},
        {2, 7,  8, 13},
        {3, 4,  9, 14}
};

static void rho(uint32_t *(v[4]), uint8_t ra, uint8_t rb)
{
    phi(A, B);
    xrot(D, A, ra);
    phi(C, D);
    xrot(B, C, rb);
}

static void f32(norx32_ctx_t *ctx)
{
    uint8_t i, j, rounds;
    uint32_t *(v[4]);
    const uint8_t *p;
    rounds = ctx->r;
    do {
        p = &g2_table[0][0];
        i = 8;
        do {
            j = 4;
            do {
                --j;
                v[j] = &ctx->s[pgm_read_byte(p++)];
            } while(j);
            rho(v, R0, R1);
            rho(v, R2, R3);
        } while (--i);
    } while (--rounds);
}

static const uint32_t init_state[] PROGMEM = {
        U0,  0,  0, U1,
         0,  0,  0,  0,
        U2, U3, U4, U5,
        U6, U7, U8 ^ (1l << 15), U9
};

static void norx32_process_block(
    norx32_ctx_t *ctx,
    const void *block,
    uint8_t tag )
{
    SET_TAG(ctx, tag);
    f32(ctx);
    memxor(ctx->s, block, RATE_BYTES);
}

static void norx32_process_last_block(
    norx32_ctx_t *ctx,
    void *out_block,
    const void *block,
    uint16_t length_b,
    uint8_t tag )
{
    while (length_b >= RATE_BITS) {
        norx32_process_block(ctx, block, tag);
        block = (uint8_t*)block + RATE_BYTES;
        length_b -= RATE_BITS;
        if (out_block) {
            memcpy(out_block, ctx->s, RATE_BYTES);
            out_block = (uint8_t*)out_block + RATE_BYTES;
        }
    }
    SET_TAG(ctx, tag);
    f32(ctx);
    memxor(ctx->s, block, (length_b + 7) / 8);
    if (out_block) {
        memcpy(out_block, ctx->s, (length_b + 7) / 8);
        out_block = (uint8_t*)out_block + (length_b + 7) / 8;
#ifndef NO_BIT_MODE
        TRUNCATE_BUFFER(out_block, length_b);
#endif
    }
#ifndef NO_BIT_MODE
    TOGGLE_BIT(ctx->s, length_b);
#else
    ((uint8_t*)ctx->s)[length_b / 8] ^= 1;
#endif
    if (length_b == RATE_BITS - 1) {
        SET_TAG(ctx, tag);
        f32(ctx);
    }
#ifndef NO_BIT_MODE
    TOGGLE_BIT(ctx->s, RATE_BITS - 1);
#else
    ((uint8_t*)ctx->s)[RATE_BYTES - 1] ^= 0x80;
#endif
}

/******************************************************************************/

int8_t norx32_init (
    norx32_ctx_t *ctx,
    const void* nonce,
    const void* key,
    uint8_t rounds,
    uint8_t parallel,
    uint16_t tag_size_b )
{
    uint32_t v;
    if (ctx == NULL || nonce == NULL || key == NULL) {
        return -1;
    }
    if (tag_size_b > 320) {
        return -1;
    }
    if (rounds < 1 || rounds > 63) {
        return -1;
    }
    if (parallel != 1) {
        return -2;
    }
    memcpy_P(ctx->s, init_state, sizeof(ctx->s));
    memcpy(&ctx->s[1], nonce, 2 * sizeof(ctx->s[1]));
    memcpy(&ctx->s[4], key, 4 * sizeof(ctx->s[4]));
    v  = ((uint32_t)rounds)   << 26;
    v ^= ((uint32_t)parallel) << 18;
    v ^= tag_size_b;
    ctx->s[14] ^= v;
    ctx->d = parallel;
    ctx->a = tag_size_b;
    ctx->r = rounds;
    f32(ctx);
    return 0;
}

void norx32_finalize(norx32_ctx_t *ctx, void *tag)
{
    SET_TAG(ctx, TAG_TAG);
    f32(ctx);
    f32(ctx);
    if (tag) {
        memcpy(tag, ctx->s, (ctx->a + 7) / 8);
#ifndef NO_BIT_MODE
        TRUNCATE_BUFFER(tag, ctx->a);
#endif
    }
}

void norx32_add_header_block(norx32_ctx_t *ctx, const void *block)
{
    norx32_process_block(ctx, block, TAG_HEADER);
}

void norx32_add_header_last_block(
    norx32_ctx_t *ctx,
    const void *block,
    uint16_t length_b )
{
    norx32_process_last_block(ctx, NULL, block, length_b, TAG_HEADER);
}

void norx32_encrypt_block(norx32_ctx_t *ctx, void *dest, const void *src)
{
    norx32_process_block(ctx, src, TAG_PAYLOAD);
    if (dest) {
        memcpy(dest, ctx->s, RATE_BYTES);
    }
}

void norx32_encrypt_last_block(
    norx32_ctx_t *ctx,
    void *dest,
    const void *src,
    uint16_t length_b )
{
    norx32_process_last_block(ctx, dest, src, length_b, TAG_PAYLOAD);
}

void norx32_add_trailer_block(norx32_ctx_t *ctx, const void *block)
{
    norx32_process_block(ctx, block, TAG_TRAILER);
}

void norx32_add_trailer_last_block(
    norx32_ctx_t *ctx,
    const void *block,
    uint16_t length_b )
{
    norx32_process_last_block(ctx, NULL, block, length_b, TAG_TRAILER);
}

/******************************************************************************/

void norx32_default_simple (
    void *data_dest,
    void *tag_dest,
    const void *key,
    const void *nonce,
    const void *header,
    size_t header_length_B,
    const void *data_src,
    size_t data_length_B,
    const void *trailer,
    size_t trailer_length_B )
{
    norx32_ctx_t ctx;
    norx32_init(&ctx, nonce, key, 4, 1, 4 * WORD_SIZE);
    if (header && header_length_B) {
        norx32_add_header_last_block(&ctx, header, header_length_B * 8);
    }
    if (data_src && data_length_B) {
        norx32_encrypt_last_block(&ctx, data_dest, data_src, data_length_B * 8);
    }
    if (trailer && trailer_length_B) {
        norx32_add_trailer_last_block(&ctx, trailer, trailer_length_B * 8);
    }
    norx32_finalize(&ctx, tag_dest);
}
