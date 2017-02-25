/*
 Simple Operating System for Smartcard Education
 Copyright (C) 2002  Matthias Bruestle <m@mbsks.franken.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* $Id: main.c,v 1.31 2002/12/24 13:33:11 m Exp $ */

/*! @file
 \brief main() function with command loop.
 */

#include <config.h>
#include <crypt.h>
#include <commands.h>
#include <sw.h>
#include <hal.h>
#include <t0.h>
#include <util/delay.h>


/*! \brief Main function containing command interpreter loop.

 At the end of the loop, sw is sent as the status word.

 This function does never return.
 */



#if defined(CTAPI)
void sosse_main( void )
#else
int main(void)
#endif
{

	//WDTCR = 0x1e;
	WDTCR = 0x10;
	//WDTCR = 0x00;


	uint8_t i, len, b,g;
	uint8_t j[1];
	uint8_t k[1];

	/* TODO: On error? */

	hal_init();

	/* Send ATR */
	/* TODO: Possible from EEPROM? */
	hal_io_sendByteT0(0x3B);
	hal_io_sendByteT0( 0xAA );

	//resplen = 0;

	/* Initialize FS state in RAM. */
	/* TODO: On error? */

	uint8_t key[32] = {
		0x6c,0xec,0xc6,0x7f,0x28,0x7d,0x08,0x3d,
		0xeb,0x87,0x66,0xf0,0x73,0x8b,0x36,0xcf,
		0x16,0x4e,0xd9,0xb2,0x46,0x95,0x10,0x90,
		0x86,0x9d,0x08,0x28,0x5d,0x2e,0x19,0x3b};

	uint8_t data[16] = {
						0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00 };


	if (!(hal_eeprom_read(&len, ATR_LEN_ADDR, 1) && (len <= ATR_MAXLEN)))
		for (;;) {
		}

	for (i = 1; i < len; i++) {
		if (!hal_eeprom_read(&b, ATR_ADDR + i, 1))
			for (;;) {
			}
		hal_io_sendByteT0(b);

	}

	unsigned long z=0;

	/* Command loop */
	for (;;) {
		header[1]=0x00;
		header[0]=0x00;
		for (i = 0; i < 5+16; i++) { //+16
			header[i] = hal_io_recByteT0();
		}

#if CONF_WITH_TRNG==1
		hal_rnd_addEntropy();
#endif

		if ((header[0] & 0xFC) == CLA_PROP) {

			switch (header[1] & 0xFE) {

			case INS_GET_RESPONSE:

				hal_rnd_getBlock(&j );

				for (g = 0; g < 16; g++) {
					data[g] = header[5+g];//;
				}

				aes_cenc( data, key,j,0 );
				break;

			case INS_READ_BINARY:
				//cmd_readBinary();
				t0_sendWord(SW_WRONG_INS);
				break;
			case INS_SELECT:
				//cmd_select();
				t0_sendWord(SW_WRONG_INS);
				break;

			case INS_UPDATE_BINARY:
				//cmd_updateBinary();
				t0_sendWord(SW_WRONG_INS);
				break;

			default:
				t0_sendWord(SW_WRONG_INS);
			}
		} else {
			t0_sendWord(SW_WRONG_CLA);
		}

#if CONF_WITH_TRNG==1
		hal_rnd_addEntropy();
#endif

		/* Return the SW in sw */
		//t0_sendSw();
	}
}




