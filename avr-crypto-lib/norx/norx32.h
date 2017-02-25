/* norx32.h */
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

#ifndef NORX_NORX32_H_
#define NORX_NORX32_H_

#include <stdint.h>

typedef struct {
    uint8_t d;
    uint8_t r;
    uint16_t a;
    uint32_t s[16];
} norx32_ctx_t;



/******************************************************************************/

int8_t norx32_init (
    norx32_ctx_t *ctx,
    const void* nonce,
    const void* key,
    uint8_t rounds,
    uint8_t parallel,
    uint16_t tag_size_b );

void norx32_finalize(norx32_ctx_t *ctx, void *tag);

void norx32_add_header_block(norx32_ctx_t *ctx, const void *block);

void norx32_add_header_last_block(
    norx32_ctx_t *ctx,
    const void *block,
    uint16_t length_b );

void norx32_encrypt_block(norx32_ctx_t *ctx, void *dest, const void *src);

void norx32_encrypt_last_block(
    norx32_ctx_t *ctx,
    void *dest,
    const void *src,
    uint16_t length_b );

void norx32_add_trailer_block(norx32_ctx_t *ctx, const void *block);

void norx32_add_trailer_last_block(
    norx32_ctx_t *ctx,
    const void *block,
    uint16_t length_b );

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
    size_t trailer_length_B );

#endif /* NORX_NORX32_H_ */
