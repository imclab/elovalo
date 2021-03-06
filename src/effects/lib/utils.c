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

/**
 * Led cube effect utilities
 */

#include "../../common/assert.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../../common/env.h"
#include "../../common/cube.h"
#include "utils.h"

/* If you are changing LED count make sure you are not using set_led
   which is optimized to work only 12-bit depths and when y and z
   dimensions have length of 8. */

/* ticks is set to ticks_volatile every time when frame calculation is
 * started. This keeps ticks stable and removes tearing. */
uint16_t ticks;

/* custom data which effects may access and which may be set at playlists */
const void *custom_data;

/* Sensor values are stored in this struct */ 
sensors_t sensors = {MAX_INTENSITY};

void set_row(uint8_t x, uint8_t z, uint8_t y1, uint8_t y2, uint16_t intensity)
{
	for(uint8_t i = y1; i <= y2; i++) {
		set_led_8_8_12(x, i, z, intensity);
	}
}

void set_z(uint8_t x, uint8_t y, uint16_t intensity)
{
	if (intensity > MAX_2D_PLOT_INTENSITY) return;

	// Do linear interpolation (two voxels per x-y pair)
	uint8_t lower_z = intensity >> GS_DEPTH;
	uint16_t upper_i = intensity & MAX_INTENSITY;
	uint16_t lower_i = MAX_INTENSITY - upper_i;
	
	set_led_8_8_12(x,y,lower_z,lower_i);
	set_led_8_8_12(x,y,lower_z + 1,upper_i);
}

void set_led_8_8_12(uint8_t x, uint8_t y, uint8_t z, uint16_t i)
{
	/* Assert (on testing environment) that we supply correct
	 * data. */
	assert(x < LEDS_X);
	assert(y < LEDS_Y);
	assert(z < LEDS_Z);
	assert(i < (1 << GS_DEPTH));

#ifdef MIRROR_X
	x = 7-x;
#endif
#ifdef MIRROR_Y
	y = 7-y;
#endif

	/* Cube buffers are bit packed: 2 voxels per 3 bytes when
	 * GS_DEPTH is 12. This calculates bit position efficiently by
	 * using bit shifts. With AVR's 8-bit registers this is
	 * optimized to do first operations with uint8_t's and do the
	 * last shift with uint16_t because it's the only one which
	 * overflows from 8 bit register. */
	const uint16_t bit_pos = 12 * (x | y << 3 | (uint16_t)z << 6);

	/* Byte position is done simply by truncating the last 8 bits
	 * of the data. Variable raw is filled with the data. */
	const uint16_t byte_pos = bit_pos >> 3;
	assert(byte_pos < GS_BUF_BYTES);
	uint16_t raw = (gs_buf_back[byte_pos] << 8) | gs_buf_back[byte_pos+1];

	/* If 12-bit value starts from the beginning of the data
	 * (bit_pos is dividable by 8) then we put the data starting
	 * from MSB, otherwise we start from MSB - 4 bits. */
	if (bit_pos & 0x7) raw = (raw & 0xf000) | i;
	else raw = (raw & 0x000f) | (i << 4);

	/* Store data back to buffer */
	gs_buf_back[byte_pos] = raw >> 8;
	gs_buf_back[byte_pos+1] = raw;
}

uint16_t get_led_wrap(int8_t x, int8_t y, int8_t z)
{
	// Assert it is not too low where we can't wrap without modulus
	assert(x >= -LEDS_X);
	assert(y >= -LEDS_Y);
	assert(z >= -LEDS_Z);

	// Assert it is not too high (for the same reason)
	assert(x < 2*LEDS_X);
	assert(y < 2*LEDS_Y);
	assert(z < 2*LEDS_Z);

	uint8_t rx = x < 0? LEDS_X + x: x >= LEDS_X? x-LEDS_X: x;
	uint8_t ry = y < 0? LEDS_Y + y: y >= LEDS_Y? y-LEDS_Y: y;
	uint8_t rz = z < 0? LEDS_Z + z: z >= LEDS_Z? z-LEDS_Z: z;

	return get_led(rx, ry, rz);
}

uint16_t get_led_8_8_12(uint8_t x, uint8_t y, uint8_t z)
{
	/* Assert (on testing environment) that we supply correct
	 * data. */
	assert(x < LEDS_X);
	assert(y < LEDS_Y);
	assert(z < LEDS_Z);

	/* Cube buffers are bit packed: 2 voxels per 3 bytes when
	 * GS_DEPTH is 12. This calculates bit position efficiently by
	 * using bit shifts. With AVR's 8-bit registers this is
	 * optimized to do first operations with uint8_t's and do the
	 * last shift with uint16_t because it's the only one which
	 * overflows from 8 bit register. */
	const uint16_t bit_pos = 12 * (x | y << 3 | (uint16_t)z << 6);

	/* Byte position is done simply by truncating the last 8 bits
	 * of the data. Variable raw is filled with the data. */
	const uint16_t byte_pos = bit_pos >> 3;
	assert(byte_pos < GS_BUF_BYTES);
	uint16_t raw = (gs_buf_front[byte_pos] << 8) | gs_buf_front[byte_pos+1];

	/* If 12-bit value starts from the beginning of the data
	 * (bit_pos is dividable by 8) then we get data starting from
	 * MSB, otherwise we get from MSB - 4 bits. */
	return (bit_pos & 0x7) ? raw & 0x0fff : raw >> 4;
}

void iterate_xy(iterate_xy_t f)
{
	for(uint8_t x = 0; x < LEDS_X; x++) {
		for(uint8_t y = 0; y < LEDS_Y; y++) {
			f(x, y);
		}
	}
}

void iterate_xyz(iterate_xyz_t f)
{
	for(uint8_t x = 0; x < LEDS_X; x++) {
		for(uint8_t y = 0; y < LEDS_Y; y++) {
			for(uint8_t z = 0; z < LEDS_Z; z++) {
				f(x, y, z);
			}
		}
	}
}

void clear_buffer(void)
{
	memset(gs_buf_back,0,GS_BUF_BYTES);
}
