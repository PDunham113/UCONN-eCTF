/* gcm128.h */
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

#ifndef GCM_GCM128_H_
#define GCM_GCM128_H_


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
#include <gcm128.h>

#define GCM128_BLOCK_BYTES 16
#define GCM128_BLOCK_BITS (GCM128_BLOCK_BYTES * 8)

#define GCM128_COUNT_BYTES 4
#define GCM128_COUNT_BITS (GCM128_COUNT_BYTES * 8)



typedef struct {
    uint8_t tag[GCM128_BLOCK_BYTES];
    uint8_t key[GCM128_BLOCK_BYTES];
} ghash128_ctx_t;

typedef struct {
    ghash128_ctx_t ghash_ctx;
    bcgen_ctx_t cipher_ctx;
    uint8_t ctr[GCM128_BLOCK_BYTES];
    uint8_t j0[GCM128_COUNT_BYTES];
    uint32_t length_a;
    uint32_t length_c;
} gcm128_ctx_t;

int8_t gcm128_init(
        gcm128_ctx_t *ctx,
        const bcdesc_t *cipher,
        const void *key,
        uint16_t key_length_b,
        const void *iv,
        uint16_t iv_length_b);

void gcm128_add_ad_block(
        gcm128_ctx_t *ctx,
        const void *block );

void gcm128_add_ad_final_block(
        gcm128_ctx_t *ctx,
        const void *block,
        uint16_t length_b );

void gcm128_encrypt_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src);

void gcm128_encrypt_final_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src,
        uint16_t length_b);

void gcm128_decrypt_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src);

void gcm128_decrypt_final_block(
        gcm128_ctx_t *ctx,
        void *dest,
        const void *src,
        uint16_t length_b);

void gcm128_finalize(
        gcm128_ctx_t *ctx,
        void *tag,
        uint16_t tag_length_b);

#endif /* GCM_GCM128_H_ */
