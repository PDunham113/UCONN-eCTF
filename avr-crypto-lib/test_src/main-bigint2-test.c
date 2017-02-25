/* main-bigint-test.c */
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
 * bigint test-suit
 * 
*/

#include "main-test-common.h"

#include "noekeon.h"
#include "noekeon_prng.h"
#include "bigint2.h"
#include "bigint2_io.h"

#include "performance_test.h"

char *algo_name = "BigInt2";

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/*****************************************************************************
 *  additional validation-functions											 *
 *****************************************************************************/
void test_echo_bigint(void) {
	bigint_t a;
	cli_putstr_P(PSTR("\r\necho test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter hex number:"));
		if (bigint_read_hex_echo(&a, 0)) {
			cli_putstr_P(PSTR("\r\n end echo test"));
			return;
		}
		cli_putstr_P(PSTR("\r\necho: "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
	}
}

void test_add_bigint(void){
	bigint_t a, b, c;
	printf_P(PSTR("\nadd test\n"));
	for (;;) {
		printf_P(PSTR("\nenter a:"));
		if (bigint_read_hex_echo(&a, 512)) {
			printf_P(PSTR("\n end add test"));
			return;
		}
		printf_P(PSTR("\nenter b:"));
		if (bigint_read_hex_echo(&b, 512)) {
			free(a.wordv);
			printf_P(PSTR("\n end add test"));
			return;
		}
		printf_P(PSTR("\n "));
		bigint_print_hex(&a);
		printf_P(PSTR(" + "));
		bigint_print_hex(&b);
		printf_P(PSTR(" = "));
		memset(&c, 0, sizeof(c));
		bigint_add_u(&c, &a, &b);
		bigint_print_hex(&c);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
		free(b.wordv);
		bigint_free(&c);
	}
}

void test_sub_bigint(void){
    bigint_t a, b, c;
    printf_P(PSTR("\nadd test\n"));
    for (;;) {
        printf_P(PSTR("\nenter a:"));
        if (bigint_read_hex_echo(&a, 512)) {
            printf_P(PSTR("\n end add test"));
            return;
        }
        printf_P(PSTR("\nenter b:"));
        if (bigint_read_hex_echo(&b, 512)) {
            free(a.wordv);
            printf_P(PSTR("\n end add test"));
            return;
        }
        printf_P(PSTR("\n "));
        bigint_print_hex(&a);
        printf_P(PSTR(" - "));
        bigint_print_hex(&b);
        printf_P(PSTR(" = "));
        memset(&c, 0, sizeof(c));
        bigint_sub_u(&c, &a, &b);
        bigint_print_hex(&c);
        cli_putstr_P(PSTR("\r\n"));
        free(a.wordv);
        free(b.wordv);
        bigint_free(&c);
    }
}

#if 0
void test_add_scale_bigint(void){
	bigint_t a, b, c;
	uint16_t scale;
	cli_putstr_P(PSTR("\r\nadd-scale test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter a:"));
		if (bigint_read_hex_echo(&a)) {
			cli_putstr_P(PSTR("\r\n end add-scale test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter b:"));
		if (bigint_read_hex_echo(&b)) {
			cli_putstr_P(PSTR("\r\n end add-scale test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter scale:"));
		{
			char str[8];
			cli_getsn_cecho(str, 7);
			scale = atoi(str);
		}
	/*
		if(bigint_read_hex_echo(&scale)){
			free(scale.wordv);
			cli_putstr_P(PSTR("\r\n end add test"));
			return;
		}
	*/
		bigint_word_t *c_b;
		c_b = malloc((MAX(a.length_W, b.length_W+scale) + 2) * sizeof(bigint_word_t));
		if(c_b==NULL){
			cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
			free(a.wordv);
			free(b.wordv);
			continue;
		}
		c.wordv = c_b;
		bigint_copy(&c, &a);
		bigint_add_scale_u(&c, &b, scale);
		cli_putstr_P(PSTR("\r\n "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR(" + "));
		bigint_print_hex(&b);
		cli_putstr_P(PSTR("<<8*"));
		cli_hexdump_rev(&scale, 2);
		cli_putstr_P(PSTR(" = "));
		bigint_print_hex(&c);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
		free(b.wordv);
		free(c_b);
	}
}
#endif


void test_mul_bigint(void){
	bigint_t a, b, c;
	cli_putstr_P(PSTR("\r\nmul test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter a:"));
		if (bigint_read_hex_echo(&a, 0)) {
			cli_putstr_P(PSTR("\r\n end mul test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter b:"));
		if (bigint_read_hex_echo(&b, 0)) {
			free(a.wordv);
			cli_putstr_P(PSTR("\r\n end mul test"));
			return;
		}
		cli_putstr_P(PSTR("\r\n "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR(" * "));
		bigint_print_hex(&b);
		cli_putstr_P(PSTR(" = "));
		bigint_word_t *c_b;
		c_b = malloc((MAX(a.length_W, b.length_W) + 1) * 2 * sizeof(bigint_word_t));
		if (c_b==NULL) {
			cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
			free(a.wordv);
			free(b.wordv);
			continue;
		}
		c.wordv = c_b;
		bigint_mul_schoolbook(&c, &a, &b);
		bigint_print_hex(&c);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
		free(b.wordv);
		free(c_b);
	}
}

#if 0
void test_mul_mont_bigint(void){
    bigint_t a, b, c, a_, b_, m_, res;
    bigint_length_t s;
    cli_putstr_P(PSTR("\r\nmul-mont test ( (a * b) % c )\r\n"));
    for (;;) {
        cli_putstr_P(PSTR("\r\nenter a:"));
        if (bigint_read_hex_echo(&a)) {
            cli_putstr_P(PSTR("\r\n end mul test"));
            return;
        }
        cli_putstr_P(PSTR("\r\nenter b:"));
        if (bigint_read_hex_echo(&b)) {
            free(a.wordv);
            cli_putstr_P(PSTR("\r\n end mul test"));
            return;
        }
        cli_putstr_P(PSTR("\r\nenter c:"));
        if (bigint_read_hex_echo(&c)) {
            free(a.wordv);
            free(b.wordv);
            cli_putstr_P(PSTR("\r\n end mul test"));
            return;
        }
        s = c.length_W;
        cli_putstr_P(PSTR("\r\n ("));
        bigint_print_hex(&a);
        cli_putstr_P(PSTR(" * "));
        bigint_print_hex(&b);
        cli_putstr_P(PSTR(") % "));
        bigint_print_hex(&c);
        cli_putstr_P(PSTR(" = "));
        bigint_word_t res_w[s], a_w_[s], b_w_[s], m_w_[s + 1];
        res.wordv = res_w;
        a_.wordv = a_w_;
        b_.wordv = b_w_;
        m_.wordv = m_w_;
        bigint_mont_gen_m_(&m_, &c);
        bigint_mont_trans(&a_, &a, &c);
        bigint_mont_trans(&b_, &b, &c);
        bigint_mont_mul(&res, &a_, &b_, &c, &m_);
        bigint_mont_red(&res, &res, &c, &m_);
        bigint_print_hex(&res);
        putchar('\n');
        free(a.wordv);
        free(b.wordv);
        free(c.wordv);
    }
}
#endif

void test_mul_word_bigint(void){
    bigint_t a, b;
    bigint_word_t *t;
    cli_putstr_P(PSTR("\r\nmul test\r\n"));
    for (;;) {
        cli_putstr_P(PSTR("\r\nenter a:"));
        if (bigint_read_hex_echo(&a, 0)) {
            cli_putstr_P(PSTR("\r\n end mul test"));
            return;
        }
        cli_putstr_P(PSTR("\r\nenter b:"));
        if (bigint_read_hex_echo(&b, 0)) {
            free(a.wordv);
            cli_putstr_P(PSTR("\r\n end mul test"));
            return;
        }
        cli_putstr_P(PSTR("\r\n "));
        bigint_print_hex(&a);
        cli_putstr_P(PSTR(" * "));
        bigint_print_hex(&b);
        cli_putstr_P(PSTR(" = "));

        if (b.length_W > 1) {
            free(a.wordv);
            free(b.wordv);
            cli_putstr_P(PSTR("\r\n end mul test"));
        }

        t = realloc(a.wordv, (a.length_W + 3) * sizeof(bigint_word_t));
        if (t == NULL) {
            cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
            free(a.wordv);
            free(b.wordv);
            continue;
        }
        a.wordv = t;
        bigint_mul_word(&a, &a, b.wordv[0]);
        bigint_print_hex(&a);
        cli_putstr_P(PSTR("\r\n"));
        free(a.wordv);
        free(b.wordv);
    }
}

void test_square_bigint(void){
	bigint_t a, c;
	cli_putstr_P(PSTR("\r\nsquare test\r\n"));
	for(;;){
		cli_putstr_P(PSTR("\r\nenter a:"));
		if(bigint_read_hex_echo(&a, 0)){
			cli_putstr_P(PSTR("\r\n end square test"));
			return;
		}
		cli_putstr_P(PSTR("\r\n "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR("**2 = "));
		bigint_word_t *c_b;
		c_b = malloc(a.length_W * 2 * sizeof(bigint_word_t));
		if(c_b == NULL){
			cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
			free(a.wordv);
			continue;
		}
		c.wordv = c_b;
		bigint_square(&c, &a);
		bigint_print_hex(&c);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
		free(c_b);
	}
}

void test_reduce_bigint(void){
	bigint_t a, b, c;
	cli_putstr_P(PSTR("\r\nreduce test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter a:"));
		if (bigint_read_hex_echo(&a, 0)) {
			cli_putstr_P(PSTR("\r\n end reduce test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter b:"));
		if (bigint_read_hex_echo(&b, 0)) {
			free(a.wordv);
			cli_putstr_P(PSTR("\r\n end reduce test"));
			return;
		}
		cli_putstr_P(PSTR("\r\n "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR(" % "));
		bigint_print_hex(&b);
		cli_putstr_P(PSTR(" = "));
		memset(&c, 0, sizeof(c));
		bigint_divide(NULL, &c, &a, &b);
		bigint_print_hex(&c);
		cli_putstr_P(PSTR("\r\n"));
        bigint_free(&c);
        bigint_free(&b);
		bigint_free(&a);
	}
}

void test_div_bigint(void){
    bigint_t a, b, c, d;
    printf_P(PSTR("\ndiv test\n"));
    for (;;) {
        printf_P(PSTR("\nenter a:"));
        if (bigint_read_hex_echo(&a, 0)) {
            printf_P(PSTR("\n end div test"));
            return;
        }
        printf_P(PSTR("\nenter b:"));
        if (bigint_read_hex_echo(&b, 0)) {
            free(a.wordv);
            printf_P(PSTR("\n end div test"));
            return;
        }
        printf_P(PSTR("\n "));
        bigint_print_hex(&a);
        printf_P(PSTR(" / "));
        bigint_print_hex(&b);
        printf_P(PSTR(" = "));
        memset(&c, 0, sizeof(c));
        memset(&d, 0, sizeof(d));
        bigint_divide(&d, &c, &a, &b);
        bigint_print_hex(&d);
        printf_P(PSTR("; R = "));
        bigint_print_hex(&c);
        printf_P(PSTR("\n"));
        bigint_free(&d);
        bigint_free(&c);
        bigint_free(&b);
        bigint_free(&a);
    }
}


#if 0
/* d = a**b % c */
void test_expmod_bigint(void){
	bigint_t a, b, c, d;
	bigint_word_t *d_b;
	cli_putstr_P(PSTR("\r\nexpnonentiation-modulo test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter a:"));
		if (bigint_read_hex_echo(&a)) {
			cli_putstr_P(PSTR("\r\n end expmod test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter b:"));
		if (bigint_read_hex_echo(&b)) {
			free(a.wordv);
			cli_putstr_P(PSTR("\r\n end expmod test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter c:"));
		if (bigint_read_hex_echo(&c)) {
			free(a.wordv);
			free(b.wordv);
			cli_putstr_P(PSTR("\r\n end expmod test"));
			return;
		}
		d_b = malloc(c.length_W * sizeof(bigint_word_t));
		if(d_b==NULL){
			cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
			free(a.wordv);
			free(b.wordv);
			free(c.wordv);
			continue;
		}
		d.wordv = d_b;
		cli_putstr_P(PSTR("\r\n "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR("**"));
		bigint_print_hex(&b);
		cli_putstr_P(PSTR(" % "));
		bigint_print_hex(&c);
		cli_putstr_P(PSTR(" = "));
		bigint_expmod_u_sam(&d, &a, &b, &c);
		bigint_print_hex(&d);
		cli_putstr_P(PSTR("\r\n"));
		free(a.wordv);
		free(b.wordv);
		free(c.wordv);
		free(d.wordv);

	}
}

/* d = a**b % c */
void test_expmod_mont_bigint(void){
    bigint_t a, b, c, d;
    bigint_word_t *d_b;
    cli_putstr_P(PSTR("\r\nexpnonentiation-modulo-montgomory test\r\n"));
    for (;;) {
        cli_putstr_P(PSTR("\r\nenter a:"));
        if (bigint_read_hex_echo(&a)) {
            cli_putstr_P(PSTR("\r\n end expmod test"));
            return;
        }
        cli_putstr_P(PSTR("\r\nenter b:"));
        if (bigint_read_hex_echo(&b)) {
            free(a.wordv);
            cli_putstr_P(PSTR("\r\n end expmod test"));
            return;
        }
        cli_putstr_P(PSTR("\r\nenter c:"));
        if (bigint_read_hex_echo(&c)) {
            free(a.wordv);
            free(b.wordv);
            cli_putstr_P(PSTR("\r\n end expmod test"));
            return;
        }
        d_b = malloc(c.length_W * sizeof(bigint_word_t));
        if (d_b == NULL) {
            cli_putstr_P(PSTR("\n\rERROR: Out of memory!"));
            free(a.wordv);
            free(b.wordv);
            free(c.wordv);
            continue;
        }
        d.wordv = d_b;
        cli_putstr_P(PSTR("\r\n "));
        bigint_print_hex(&a);
        cli_putstr_P(PSTR("**"));
        bigint_print_hex(&b);
        cli_putstr_P(PSTR(" % "));
        bigint_print_hex(&c);
        cli_putstr_P(PSTR(" = "));
        bigint_expmod_u_mont_sam(&d, &a, &b, &c);
        bigint_print_hex(&d);
        cli_putstr_P(PSTR("\r\n"));
        free(a.wordv);
        free(b.wordv);
        free(c.wordv);
        free(d.wordv);

    }
}

#endif

void test_gcdext_bigint(void){
	bigint_t a, b, c, d, e;
	cli_putstr_P(PSTR("\r\ngcdext test\r\n"));
	for (;;) {
		cli_putstr_P(PSTR("\r\nenter a:"));
		if (bigint_read_hex_echo(&a, 0)) {
			cli_putstr_P(PSTR("\r\n end gcdext test"));
			return;
		}
		cli_putstr_P(PSTR("\r\nenter b:"));
		if (bigint_read_hex_echo(&b, 0)) {
			bigint_free(&a);
			cli_putstr_P(PSTR("\r\n end gcdext test"));
			return;
		}

		memset(&c, 0, sizeof(c));
        memset(&d, 0, sizeof(d));
        memset(&e, 0, sizeof(e));
		cli_putstr_P(PSTR("\r\n gcdext( "));
		bigint_print_hex(&a);
		cli_putstr_P(PSTR(", "));
		bigint_print_hex(&b);
		cli_putstr_P(PSTR(") => "));
		bigint_gcdext(&c, &d, &e, &a, &b);
		cli_putstr_P(PSTR("a = "));
		bigint_print_hex(&d);
		cli_putstr_P(PSTR("; b = "));
		bigint_print_hex(&e);
		cli_putstr_P(PSTR("; gcd = "));
		bigint_print_hex(&c);

		cli_putstr_P(PSTR("\r\n"));
		bigint_free(&a);
        bigint_free(&b);
        bigint_free(&c);
        bigint_free(&d);
        bigint_free(&e);
	}
}

void testrun_performance_bigint(void){

}
/*****************************************************************************
 *  main																	 *
 *****************************************************************************/

const char echo_test_str[]        PROGMEM = "echo-test";
const char add_test_str[]         PROGMEM = "add-test";
const char sub_test_str[]         PROGMEM = "sub-test";
const char add_scale_test_str[]   PROGMEM = "add-scale-test";
const char mul_test_str[]         PROGMEM = "mul-test";
const char mul_mont_test_str[]    PROGMEM = "mul-mont-test";
const char mul_word_test_str[]    PROGMEM = "mul-word-test";
const char square_test_str[]      PROGMEM = "square-test";
const char reduce_test_str[]      PROGMEM = "reduce-test";
const char div_test_str[]         PROGMEM = "div-test";
const char expmod_test_str[]      PROGMEM = "expmod-test";
const char expmod_mont_test_str[] PROGMEM = "expmod-mont-test";
const char gcdext_test_str[]      PROGMEM = "gcdext-test";
const char quick_test_str[]       PROGMEM = "quick-test";
const char performance_str[]      PROGMEM = "performance";
const char echo_str[]             PROGMEM = "echo";

const cmdlist_entry_t cmdlist[] PROGMEM = {
	{ add_test_str,         NULL, test_add_bigint               },
    { sub_test_str,         NULL, test_sub_bigint               },
//	{ add_scale_test_str,   NULL, test_add_scale_bigint         },
	{ mul_test_str,         NULL, test_mul_bigint               },
//    { mul_mont_test_str,    NULL, test_mul_mont_bigint          },
    { mul_word_test_str,    NULL, test_mul_word_bigint          },
	{ square_test_str,      NULL, test_square_bigint            },
    { reduce_test_str,      NULL, test_reduce_bigint            },
	{ div_test_str,         NULL, test_div_bigint               },
//    { expmod_test_str,      NULL, test_expmod_bigint            },
//    { expmod_mont_test_str, NULL, test_expmod_mont_bigint       },
	{ gcdext_test_str,      NULL, test_gcdext_bigint            },
//	{ quick_test_str,       NULL, test_gcdext_simple            },
	{ echo_test_str,        NULL, test_echo_bigint              },
	{ performance_str,      NULL, testrun_performance_bigint    },
	{ echo_str,         (void*)1, (void_fpt)echo_ctrl           },
	{ NULL,                 NULL, NULL                          }
};

int main (void){
    main_setup();
    int_realloc = realloc;
    int_free = free;
    for(;;){
        welcome_msg(algo_name);
		cmd_interface(cmdlist);
	}
}
