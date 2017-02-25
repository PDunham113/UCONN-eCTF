/* main-norx-test.c */
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


/* main-arcfour-test.c */
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
 * arcfour (RC4 compatible) test-suit
 *
*/


#include "main-test-common.h"

#include <norx32.h>
#include "performance_test.h"

char *algo_name = "norx";

/*****************************************************************************
 *  additional validation-functions                                          *
 *****************************************************************************/

#define DUMP(x) do { printf("%s", "\n\n" #x ":"); \
                    cli_hexdump_block((x), sizeof(x), 4, 16); } while (0)

#if 0
void g32(uint32_t *(a[4]));
void f32(norx32_ctx_t *ctx);


void g32_dump(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t *(x[4]) = {&a, &b, &c, &d};
    printf("\n (a,b,c,d) = (%08lX, %08lX, %08lX, %08lX)", *(x[0]), *(x[1]), *(x[2]), *(x[3]));
    g32(x);
    printf("\nG(a,b,c,d) = (%08lX, %08lX, %08lX, %08lX)\n", *(x[0]), *(x[1]), *(x[2]), *(x[3]));
}

void testrun_g32(void)
{
    uint32_t x;
    x = 1;
    g32_dump(0, 0, 0, 0);
    g32_dump(x, 0, 0, 0);
    g32_dump(0, x, 0, 0);
    g32_dump(0, 0, x, 0);
    g32_dump(0, 0, 0, x);
    x = 0x80000000l;
    g32_dump(x, 0, 0, 0);
    g32_dump(0, x, 0, 0);
    g32_dump(0, 0, x, 0);
    g32_dump(0, 0, 0, x);
    x = 0xffffffffl;
    g32_dump(x, x, x, x);

    g32_dump(0x01234567l, 0x89abcdefl, 0xfedcba98l, 0x76543210l);
}

void testrun_f32(void)
{
    norx32_ctx_t ctx;
    memset(ctx.s, 0, sizeof(ctx.s));
    ctx.s[0] = 1;
    ctx.r = 8;
    f32(&ctx);
}
#endif

void testrun_norx32(void)
{
    const uint8_t key[] = {
        0x33, 0x22, 0x11, 0x00,
        0x77, 0x66, 0x55, 0x44,
        0xBB, 0xAA, 0x99, 0x88,
        0xFF, 0xEE, 0xDD, 0xCC,
    };
    const uint8_t nonce[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    const uint8_t header[] = {
        0x02, 0x00, 0x00, 0x10,
        0x04, 0x00, 0x00, 0x30,
    };
    const uint8_t payload[] = {
            0x07, 0x00, 0x00, 0x80,
            0x05, 0x00, 0x00, 0x60,
            0x03, 0x00, 0x00, 0x40,
            0x01, 0x00, 0x00, 0x20,
    };
//    const uint8_t trailer[0];
    uint8_t crypt[16];
    uint8_t tag[16];
    norx32_default_simple(
            crypt,
            tag,
            key,
            nonce,
            header,
            sizeof(header),
            payload,
            sizeof(payload),
            NULL,
            0 );
    DUMP(key);
    DUMP(nonce);
    DUMP(header);
    DUMP(payload);
//    DUMP(trailer);
    DUMP(crypt);
    DUMP(tag);
/*
    cli_hexdump_block(crypt, sizeof(payload), 4, 16);
    cli_hexdump_block(tag, sizeof(tag), 4, 16);
*/
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
    { test_str,        NULL, testrun_norx32},
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


