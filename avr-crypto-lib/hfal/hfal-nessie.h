/* hfal-nessie.h */
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
 * \file    hfal-nessie.h
 * \author  Daniel Otte
 * \email   bg@nerilex.org
 * \date    2009-05-10
 * \license GPLv3 or later
 * 
 */

#ifndef HFAL_NESSIE_H_
#define HFAL_NESSIE_H_

#include "hashfunction_descriptor.h"

void hfal_nessie(const hfdesc_t *hd);
void hfal_nessie_multiple(const hfdesc_t *const *hd_list);

#endif /* HFAL_NESSIE_H_ */
