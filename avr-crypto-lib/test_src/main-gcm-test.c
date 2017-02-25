/* main-gcm-test.c */
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


/*
 * GCM test-suit
 *
*/


#include "main-test-common.h"

#include <gcm128.h>
#include <bcal_aes128_enconly.h>
#include <bcal_aes192_enconly.h>
#include <bcal_aes256_enconly.h>
#include "performance_test.h"

char *algo_name = "GCM-AES128";

/*****************************************************************************
 *  additional validation-functions                                          *
 *****************************************************************************/

#define DUMP_LEN(x, len) do { printf("%s", "\n\n" #x ":"); \
                    cli_hexdump_block((x), (len), 4, 16); } while (0)

#define DUMP(x) DUMP_LEN(x, sizeof(x))

#define elementsof(t) (sizeof(t) / sizeof(t[0]))

/*****************************************************************************
 *                                                                           *
 *****************************************************************************/

const uint8_t zero_block[] PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t key_b[] PROGMEM = {
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};


const uint8_t ad_block[] PROGMEM = {
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xab, 0xad, 0xda, 0xd2
};

const uint8_t pt_block[] PROGMEM = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
};

/*****************************************************************************/

const uint8_t iv_block[] PROGMEM = {
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88
};

/*****************************************************************************/

const uint8_t iv_6[] PROGMEM = {
        0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
        0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
        0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
        0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
        0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
        0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
        0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
        0xa6, 0x37, 0xb3, 0x9b
};

typedef struct {
    const void *key;
    uint16_t key_length_b;
    const void *iv;
    uint16_t iv_length_b;
    const void *ad;
    uint16_t ad_length_b;
    const void *pt;
    uint16_t pt_length_b;
} test_t;

const test_t test_set[] PROGMEM = {
        { zero_block, 128, zero_block, 96, NULL, 0, NULL, 0 },
        { zero_block, 128, zero_block, 96, NULL, 0, zero_block, 128 },
        { key_b, 128, iv_block, 96, NULL, 0, pt_block, 512 },
        { key_b, 128, iv_block, 96, ad_block, 160, pt_block, 480 },
        { key_b, 128, iv_block, 64, ad_block, 160, pt_block, 480 },
        { key_b, 128, iv_6, sizeof(iv_6) * 8, ad_block, 160, pt_block, 480 },

        { zero_block, 192, zero_block, 96, NULL, 0, NULL, 0 },
        { zero_block, 192, zero_block, 96, NULL, 0, zero_block, 128 },
        { key_b, 192, iv_block, 96, NULL, 0, pt_block, 512 },
        { key_b, 192, iv_block, 96, ad_block, 160, pt_block, 480 },
        { key_b, 192, iv_block, 64, ad_block, 160, pt_block, 480 },
        { key_b, 192, iv_6, sizeof(iv_6) * 8, ad_block, 160, pt_block, 480 },

        { zero_block, 256, zero_block, 96, NULL, 0, NULL, 0 },
        { zero_block, 256, zero_block, 96, NULL, 0, zero_block, 128 },
        { key_b, 256, iv_block, 96, NULL, 0, pt_block, 512 },
        { key_b, 256, iv_block, 96, ad_block, 160, pt_block, 480 },
        { key_b, 256, iv_block, 64, ad_block, 160, pt_block, 480 },
        { key_b, 256, iv_6, sizeof(iv_6) * 8, ad_block, 160, pt_block, 480 },
};

/*****************************************************************************
 *                                                                           *
 *****************************************************************************/


int8_t gcm128_simple(
        const void *key,
        const void *iv,
        uint16_t iv_length_b,
        const void * ad,
        uint16_t ad_length_b,
        void *dest,
        const void *src,
        uint16_t src_length_b,
        void *tag,
        uint8_t tag_length_b)
{
    gcm128_ctx_t ctx;
    DUMP_LEN(key, 16);
    DUMP_LEN(iv, (iv_length_b + 7) / 8);
    DUMP_LEN(ad, (ad_length_b + 7) / 8);
    DUMP_LEN(src, (src_length_b + 7) / 8);
    if (gcm128_init(&ctx, &aes128_desc, key, 128, iv, iv_length_b)) {
        return -1;
    }
    gcm128_add_ad_final_block(&ctx, ad, ad_length_b);
    gcm128_encrypt_final_block(&ctx, dest, src, src_length_b);
    gcm128_finalize(&ctx, tag, tag_length_b);
    if (dest) {
        DUMP_LEN(dest, (src_length_b + 7) / 8);
    }
    DUMP_LEN(tag, (tag_length_b + 7) / 8);
    return 0;
}

int8_t gcm128_simple_progmem(
        const void *key_p,
        uint16_t key_length_b,
        const void *iv_p,
        uint16_t iv_length_b,
        const void * ad_p,
        uint16_t ad_length_b,
        const void *src_p,
        uint16_t src_length_b,
        void *tag,
        uint8_t tag_length_b)
{
    uint8_t dec_tag[16];
    int8_t r;
    gcm128_ctx_t ctx, dec_ctx;
    const bcdesc_t *cipher;
    switch (key_length_b) {
    case 128: cipher = &aes128_desc; break;
    case 192: cipher = &aes192_desc; break;
    case 256: cipher = &aes256_desc; break;
    default: return -1;
    }
    {
        uint8_t key[key_length_b / 8];
        uint8_t iv[(iv_length_b + 7) / 8];
        memcpy_P(key, key_p, key_length_b / 8);
        memcpy_P(iv, iv_p, (iv_length_b + 7) / 8);
        if ((r = gcm128_init(&ctx, cipher, key, key_length_b, iv, iv_length_b))) {
            printf_P(PSTR("DBG: (Oooops) Error: %"PRId8"\n"), r);
            uart0_flush();
            return -1;
        }
        if ((r = gcm128_init(&dec_ctx, cipher, key, key_length_b, iv, iv_length_b))) {
            printf_P(PSTR("DBG: (Oooops) Error: %"PRId8"\n"), r);
            uart0_flush();
            return -1;
        }

    }
    uint8_t tmp[GCM128_BLOCK_BYTES];
    while (ad_length_b >= GCM128_BLOCK_BITS) {
        memcpy_P(tmp, ad_p, GCM128_BLOCK_BYTES);
        ad_p = &((uint8_t*)ad_p)[GCM128_BLOCK_BYTES];
        ad_length_b -= GCM128_BLOCK_BITS;
        gcm128_add_ad_block(&ctx, tmp);
        gcm128_add_ad_block(&dec_ctx, tmp);
    }
    memcpy_P(tmp, ad_p, (ad_length_b + 7) / 8);
    gcm128_add_ad_final_block(&ctx, tmp, ad_length_b);
    gcm128_add_ad_final_block(&dec_ctx, tmp, ad_length_b);

    while (src_length_b >= GCM128_BLOCK_BITS) {
        memcpy_P(tmp, src_p, GCM128_BLOCK_BYTES);
        src_length_b -= GCM128_BLOCK_BITS;
        gcm128_encrypt_block(&ctx, tmp, tmp);
        gcm128_decrypt_block(&dec_ctx, tmp, tmp);
        if (memcmp_P(tmp, src_p, GCM128_BLOCK_BYTES)) {
            printf("DBG: Error: decryption error");
            DUMP(tmp);
        }
//        DUMP(tmp);
        src_p = &((uint8_t*)src_p)[GCM128_BLOCK_BYTES];
    }
    memcpy_P(tmp, src_p, (src_length_b + 7) / 8);
    gcm128_encrypt_final_block(&ctx, tmp, tmp, src_length_b);
    gcm128_decrypt_final_block(&dec_ctx, tmp, tmp, src_length_b);
    if (src_length_b > 0) {
//        DUMP_LEN(tmp, (src_length_b + 7) / 8);
        if (memcmp_P(tmp, src_p, (src_length_b + 7) / 8)) {
            printf("DBG: Error: decryption error");
            DUMP_LEN(tmp, (src_length_b + 7) / 8);
        }
    }
    gcm128_finalize(&dec_ctx, dec_tag, tag_length_b);
    gcm128_finalize(&ctx, tag, tag_length_b);
    if (memcmp(tag, dec_tag, (tag_length_b + 7) / 8)) {
        printf("DBG: Error: tag error");
        DUMP_LEN(tag, (tag_length_b + 7) / 8);
        DUMP_LEN(dec_tag, (tag_length_b + 7) / 8);
    }
    return 0;
}

void testrun_gcm128(void)
{
    uint8_t key[16];
    uint8_t iv[12];
    uint8_t tag[16];
    uint8_t plain[16];
    uint8_t cipher[16];
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    memset(plain, 0, sizeof(plain));
    gcm128_simple(key, iv, 96, NULL, 0, NULL, NULL, 0, tag, 128);
    gcm128_simple(key, iv, 96, NULL, 0, cipher, plain, 128, tag, 128);
}



void testrun_gcm128_progmem(void)
{
    test_t t;
    uint8_t tag[16];
    uint8_t i;
    for (i = 0; i < elementsof(test_set); ++i) {
        printf_P(PSTR("== Test %"PRId8" ==\n"), i + 1);
        uart0_flush();
        memcpy_P(&t, &test_set[i], sizeof(t));
        gcm128_simple_progmem(
                t.key, t.key_length_b,
                t.iv, t.iv_length_b,
                t.ad, t.ad_length_b,
                t.pt, t.pt_length_b,
                tag, 128);
        DUMP(tag);
        puts("\n\n");
        uart0_flush();
    }
}

/*****************************************************************************
 *  main                                                                     *
 *****************************************************************************/

const char nessie_str[]      PROGMEM = "nessie";
const char test_str[]        PROGMEM = "test";
const char ftest_str[]       PROGMEM = "ftest";
const char gtest_str[]       PROGMEM = "gtest";
const char performance_str[] PROGMEM = "performance";
const char echo_str[]        PROGMEM = "echo";

const cmdlist_entry_t cmdlist[] PROGMEM = {
//    { nessie_str,      NULL, NULL },
    { test_str,        NULL, testrun_gcm128_progmem},
//    { ftest_str,       NULL, testrun_f32},
//    { gtest_str,       NULL, testrun_g32},
//    { performance_str, NULL, testrun_performance_arcfour},
    { echo_str,    (void*)1, (void_fpt)echo_ctrl},
    { NULL,            NULL, NULL}
};

int main(void) {
    main_setup();

    for(;;){
        welcome_msg(algo_name);
        cmd_interface(cmdlist);
    }

}


