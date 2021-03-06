/* -*- mode: c; c-file-style: "linux" -*-
 *  vi: set shiftwidth=8 tabstop=8 noexpandtab:
 *
 *  Copyright 2012 Elovalo project group 
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  POSIX compatible real time functions. Remember that time_t may be
 *  64 bit on POSIX platforms but is uint32_t in AVR.
 */
#ifdef AVR
#include "../avr/clock.h"
#else
#include <time.h>
#endif

#include <stdint.h>

/**
 * Gets timezone as second offset.
 */
int32_t get_timezone(void);

/**
 * Set timezone as second offset and writes it to EEPROM.
 */
void set_timezone(int32_t tz);
