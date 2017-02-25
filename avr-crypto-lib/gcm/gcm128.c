/* gcm128.c */
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
#include <stdlib.h>
#include <string.h>
#include <memxor.h>
#include <blockcipher_descriptor.h>
#include <bcal-basic.h>
#include <gcm128.h>

#include <stdio.h>
#include <cli.h>
#include <uart.h>

#define DUMP_LEN_LINE(x, len, line) do { \
        printf("\n\n (DBG <" __FILE__ " %s " #line ">)" #x ":", __func__); \
        cli_hexdump_block((x), (len), 4, 16); \
        uart0_flush(); } while (0)

#define DUMP_dummy(x, len, v) DUMP_LEN_LINE(x, len, v)
#define DUMP_LEN(x, len) DUMP_dummy(x, len, __LINE__)


#define DUMP(x) DUMP_LEN(x, sizeof(x))


#define BLOCK_BYTES GCM128_BLOCK_BYTES
#define BLOCK_BITS GCM128_BLOCK_BITS

#define COUNT_BYTES GCM128_COUNT_BYTES
#define COUNT_BITS GCM128_COUNT_BITS

#define POLY_BYTE 0xE1


static uint8_t shift_block_right(void *a)
{
    uint8_t c1 = 0, c2;
    uint8_t i = BLOCK_BYTES;
    uint8_t *p = a;
    do {
        c2 = *p & 1;
        *p = (*p >> 1) | c1;
        p++;
        c1 = c2 ? 0x80 : 0;
    } while (--i);
    return c2;
}

static void gmul128(
        void *dest,
        const void *a,
        const void *b)
{
    uint8_t c, v[BLOCK_BYTES], t, dummy[BLOCK_BYTES + 1], *(lut[2]);
    const uint8_t *x = b;
    uint8_t i, j;
    memset(dest, 0, BLOCK_BYTES);
    memset(dummy, 0, BLOCK_BYTES);
    dummy[BLOCK_BYTES] = POLY_BYTE;
    memcpy(v, a, BLOCK_BYTES);
    lut[0] = dummy;
    lut[1] = v;
    i = BLOCK_BYTES;
    do {
        j = 8;
        t = *x++;
        do {
            memxor(dest, lut[t >> 7], BLOCK_BYTES);
            t <<= 1;
            c = shift_block_right(v);
            v[0] ^= dummy[15 + c];
        } while (--j);
    } while (--i);
}

static void ghash128_init(ghash128_ctx_t *ctx)
{
    memset(ctx->tag, 0, 16);
}

static void ghash128_block(
        ghash128_ctx_t *ctx,
        const void *block)
{
    uint8_t tmp[BLOCK_BYTES];
    memcpy(tmp, ctx->tag, BLOCK_BYTES);
    memxor(tmp, block, BLOCK_BYTES);
    gmul128(ctx->tag, tmp, ctx->key);
}

static void inc32(void *a)
{
    uint8_t c, *p = a;
    c = (p[3]++ == 0);
    c &= ((p[2] += c) == 0);
    c &= ((p[1] += c) == 0);
    p[0] +=c;
}

int8_t gcm128_init(
        gcm128_ctx_t *ctx,
        const bcdesc_t *cipher,
        const void *key,
        uint16_t key_length_b,
        const void *iv,
        uint16_t iv_length_b)
{
    uint8_t r;
    uint8_t tmp[BLOCK_BYTES];
    if (bcal_cipher_getBlocksize_b(cipher) != BLOCK_BITS) {
        return -1;
    }
    if ((r = bcal_cipher_init(cipher, key, key_length_b, &ctx->cipher_ctx))) {
        printf_P(PSTR("Error: return code: %"PRId8" key length: %"PRId16" <%s %s %d>\n"), r, key_length_b, __FILE__, __func__, __LINE__);
        uart0_flush();
        return -2;
    }
    memset(ctx->ghash_ctx.key, 0, BLOCK_BYTES);
    bcal_cipher_enc(ctx->ghash_ctx.key, &ctx->cipher_ctx);
    ghash128_init(&ctx->ghash_ctx);
    if (iv_length_b == BLOCK_BITS - COUNT_BITS) {
        memcpy(ctx->ctr, iv, (BLOCK_BITS - COUNT_BITS) / 8);
        memset(&ctx->ctr[BLOCK_BYTES - COUNT_BYTES], 0, COUNT_BYTES - 1);
        ctx->ctr[BLOCK_BYTES - 1] = 1;
    } else {
        uint16_t ctr = iv_length_b / BLOCK_BITS;
        while (ctr--)
        {
            ghash128_block(&ctx->ghash_ctx, iv);
            iv = &((uint8_t*)iv)[BLOCK_BYTES];
        }
        memset(tmp, 0, BLOCK_BYTES);
        memcpy(tmp, iv, (iv_length_b % BLOCK_BITS + 7) / 8);
        if (iv_length_b & 7) {
            tmp[(iv_length_b % BLOCK_BITS) / 8] &= 0xff << (8 - (iv_length_b & 7));
        }
        ghash128_block(&ctx->ghash_ctx, tmp);
        memset(tmp, 0, BLOCK_BYTES);
        tmp[BLOCK_BYTES - 2] = iv_length_b >> 8;
        tmp[BLOCK_BYTES - 1] = iv_length_b & 0xff;
        ghash128_block(&ctx->ghash_ctx, tmp);
        memcpy(ctx->ctr, ctx->ghash_ctx.tag, BLOCK_BYTES);
        ghash128_init(&ctx->ghash_ctx);
    }
    memcpy(ctx->j0, &ctx->ctr[BLOCK_BYTES - COUNT_BYTES], COUNT_BYTES);
    ctx->length_a = 0;
    ctx->length_c = 0;
    return 0;
}

void gcm128_add_ad_block(
        gcm128_ctx_t *ctx,
        const void *block )
{
    ghash128_block(&ctx->ghash_ctx, block);
    ctx->length_a += BLOCK_BITS;
}

void gcm128_add_ad_final_block(
        gcm128_ctx_t *ctx,
        const void *block,
        uint16_t length_b )
{
    uint8_t tmp[BLOCK_BYTES];
    while (length_b >= BLOCK_BITS)
    {
        gcm128_add_ad_block(ctx, block);
        length_b -= BLOCK_BITS;
        block = &((uint8_t*)block)[BLOCK_BYTES];
    }
    if (length_b > 0) {
        memset(tmp, 0, BLOCK_BYTES);
        memcpy(tmp, block, (length_b + 7) / 8);
        if (length_b & 7) {
            tmp[length_b / 8] &= 0xff << (8 - (length_b & 7));
        }
        ghash128_block(&ctx->ghash_ctx, tmp);
        ctx->length_a += length_b;
    }
}

void gcm128_encrypt_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src)
{
    uint8_t tmp[BLOCK_BYTES];
    inc32(&ctx->ctr[BLOCK_BYTES - COUNT_BYTES]);
    memcpy(tmp, ctx->ctr, BLOCK_BYTES);
    bcal_cipher_enc(tmp, &ctx->cipher_ctx);
    memxor(tmp, src, BLOCK_BYTES);
    ghash128_block(&ctx->ghash_ctx, tmp);
    ctx->length_c += BLOCK_BITS;
    if (dest) {
        memcpy(dest, tmp, BLOCK_BYTES);
    }
}

void gcm128_encrypt_final_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src,
        uint16_t length_b
        )
{
    uint8_t tmp[BLOCK_BYTES];
    while (length_b >= BLOCK_BITS) {
        gcm128_encrypt_block(ctx, dest, src);
        length_b -= BLOCK_BITS;
        if (dest) {
            dest = &((uint8_t*)dest)[BLOCK_BYTES];
        }
        src = &((uint8_t*)src)[BLOCK_BYTES];
    }
    if (length_b > 0) {
        inc32(&ctx->ctr[BLOCK_BYTES - COUNT_BYTES]);
        memcpy(tmp, ctx->ctr, BLOCK_BYTES);
        bcal_cipher_enc(tmp, &ctx->cipher_ctx);
        memxor(tmp, src, BLOCK_BYTES);
        memset(&tmp[(length_b + 7) / 8], 0, BLOCK_BYTES - (length_b + 7) / 8);
        if (length_b & 7) {
            tmp[length_b / 8] &= 0xff << (8 - (length_b & 7));
        }
        ghash128_block(&ctx->ghash_ctx, tmp);
        ctx->length_c += length_b;
        if (dest) {
            memcpy(dest, tmp, (length_b + 7) / 8);
        }
    }
}

void gcm128_decrypt_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src)
{
    uint8_t tmp[BLOCK_BYTES];
    ghash128_block(&ctx->ghash_ctx, src);
    inc32(&ctx->ctr[BLOCK_BYTES - COUNT_BYTES]);
    memcpy(tmp, ctx->ctr, BLOCK_BYTES);
    bcal_cipher_enc(tmp, &ctx->cipher_ctx);
    memxor(tmp, src, BLOCK_BYTES);
    ctx->length_c += BLOCK_BITS;
    if (dest) {
        memcpy(dest, tmp, BLOCK_BYTES);
    }
}

void gcm128_decrypt_final_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src,
        uint16_t length_b)
{
    uint8_t tmp[BLOCK_BYTES];
    while (length_b > BLOCK_BITS) {
        gcm128_decrypt_block(ctx, dest, src);
        length_b -= BLOCK_BITS;
        if (dest) {
            dest = &((uint8_t*)dest)[BLOCK_BYTES];
        }
        src = &((uint8_t*)src)[BLOCK_BYTES];
    }
    if (length_b > 0) {
        memcpy(tmp, src, (length_b + 7) / 8);
        memset(&tmp[(length_b + 7) / 8], 0, BLOCK_BYTES - (length_b + 7) / 8);
        if (length_b & 7) {
            tmp[length_b / 8] &= 0xff << (8 - (length_b & 7));
        }
        ghash128_block(&ctx->ghash_ctx, tmp);
        inc32(&ctx->ctr[BLOCK_BYTES - COUNT_BYTES]);
        memcpy(tmp, ctx->ctr, BLOCK_BYTES);
        bcal_cipher_enc(tmp, &ctx->cipher_ctx);
        memxor(tmp, src, BLOCK_BYTES);
        memset(&tmp[(length_b + 7) / 8], 0, BLOCK_BYTES - (length_b + 7) / 8);
        if (length_b & 7) {
            tmp[length_b / 8] &= 0xff << (8 - (length_b & 7));
        }
        ctx->length_c += length_b;
        if (dest) {
            memcpy(dest, tmp, (length_b + 7) / 8);
        }
    }
}

void gcm128_finalize(gcm128_ctx_t *ctx, void *tag, uint16_t tag_length_b)
{
    uint8_t tmp[BLOCK_BYTES];
    memset(tmp, 0, BLOCK_BYTES);
    tmp[4] = ctx->length_a >> 24;
    tmp[5] = ctx->length_a >> 16;
    tmp[6] = ctx->length_a >>  8;
    tmp[7] = ctx->length_a;
    tmp[12] = ctx->length_c >> 24;
    tmp[13] = ctx->length_c >> 16;
    tmp[14] = ctx->length_c >>  8;
    tmp[15] = ctx->length_c;
    ghash128_block(&ctx->ghash_ctx, tmp);
    memcpy(tmp, ctx->ctr, BLOCK_BYTES - COUNT_BYTES);
    memcpy(&tmp[BLOCK_BYTES - COUNT_BYTES], ctx->j0, COUNT_BYTES);
    bcal_cipher_enc(tmp, &ctx->cipher_ctx);
    bcal_cipher_free(&ctx->cipher_ctx);
    memxor(tmp, ctx->ghash_ctx.tag, BLOCK_BYTES);
    if (tag_length_b > BLOCK_BITS) {
        tag_length_b = BLOCK_BITS;
    }
    if (tag_length_b & 7) {
        tmp[tag_length_b / 8] &= 0xff << (8 - (tag_length_b & 7));
    }
    if (tag) {
        memcpy(tag, tmp, (tag_length_b + 7) / 8);
    }
}
