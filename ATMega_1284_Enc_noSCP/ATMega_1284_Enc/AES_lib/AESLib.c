/*
    This file is part of the aeslib.
    Copyright (C) 2012 Davy Landman (davy.landman@gmail.com) 

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
#include "AESLib.h"
#include <stdint.h>
#include "aes.h"
#include <avr/pgmspace.h>



// encrypt single 128bit block. data is assumed to be 16 uint8_t's
// key is assumed to be 256bit thus 32 uint8_t's
void aes256_enc_single(const uint8_t* key, void* data){
	aes256_ctx_t ctx;
	aes256_init(key, &ctx);
	aes256_enc(data, &ctx);
}
