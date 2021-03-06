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

#ifndef EFFECT_UTILS_H
#define EFFECT_UTILS_H

#include <stdbool.h>
#include "../../common/env.h"

/* Defining set_led() as a macro which chooses the most efficient
 * implementation available */
#if LEDS_X == 8 && LEDS_Y == 8 && GS_DEPTH == 12 && BYTES_PER_LAYER == 96
#define set_led(x,y,z,i) set_led_8_8_12(x,y,z,i)
#define get_led(x,y,z) get_led_8_8_12(x,y,z)
#else
#error "There is no set_led() implementation for this geometry"
#endif

/* Maximum intensity returned from the 2D plotting function */
#define MAX_2D_PLOT_INTENSITY ((LEDS_Z-1)*(1 << GS_DEPTH)-1)

/* Generates wrapper function for two dimensional plots to make the
 * implementations much simpler */
#define XY(wrap)						\
  static void wrap##_kernel(uint8_t x, uint8_t y);		\
  static void wrap(void){clear_buffer();iterate_xy(&wrap##_kernel);}	\
  static void wrap##_kernel(uint8_t x, uint8_t y)

/* Generates wrapper function for three dimensional plots to make
 * the implementations much simpler */
#define XYZ(wrap)						\
  static void wrap##_kernel(uint8_t x, uint8_t y, uint8_t z);		\
  static void wrap(void){clear_buffer();iterate_xyz(&wrap##_kernel);}	\
  static void wrap##_kernel(uint8_t x, uint8_t y, uint8_t z)

/* Arbitary drawing function. The implementation is responsible to set
 * pixels by itself. Buffer flipping is done outside this function. */
typedef void(*draw_t)(void);

/* Effect initializator type. The initializator is run before any
 * drawing is done on that effect. */
typedef void(*init_t)(void);

/* This structure holds information about the effects and how to draw
 * them. */
typedef struct {
	const char *name;      // Name for effect. Used in file dumps.
	init_t init;           // Initializatior, may be NULL.
	draw_t draw;           // Drawing function, run once per buffer swap.
	uint8_t flip_buffers;  // Flip buffers during execution.
	uint8_t minimum_ticks; // Minimum amount of ticks per draw.
	bool dynamic_text;     // Contains dynamic custom data?
} effect_t;

#define NO_FLIP 0
#define FLIP 1

typedef struct {
	uint16_t debug_value; // Settable via serial port only. TODO: to be removed
	uint8_t distance1;
	uint8_t distance2;
	uint8_t ambient_light;
	uint8_t sound_pressure_level;
} sensors_t;

// XXX: might want to replace flipBuffers with a set of bitfields
// if more flags are needed

/**
 * Sets row x, z, y1, y2 to given intensity. See below set_led for more details.
 */
void set_row(uint8_t x, uint8_t z, uint8_t y1, uint8_t y2, uint16_t intensity);

/**
 * Do linear interpolation and set z coordinate accordingly. Intensity
 * must be between 0 and MAX_2D_PLOT_INTENSITY, inclusively.
 */
void set_z(uint8_t x, uint8_t y, uint16_t intensity);

/**
 * Sets led intensity. i is the intensity of the LED in range
 * 0..4095. This implementation is AVR optimized and handles only
 * cases where LEDS_X and LEDS_Y are 8, GS_DEPTH is 12, and layer has
 * no padding. Do not call directly, use set_led() instead.
 */
void set_led_8_8_12(uint8_t x, uint8_t y, uint8_t z, uint16_t i);

/**
 * Gets led intensity. Wraps around bounds.
 */
uint16_t get_led_wrap(int8_t x, int8_t y, int8_t z);

/**
 * Gets led intensity from front buffer. Returns intensity of a LED in
 * range 0..4095.  This implementation is AVR optimized and handles
 * only cases where LEDS_X and LEDS_Y are 8, GS_DEPTH is 12, and layer
 * has no padding. Do not call directly, use get_led() instead.
 */
uint16_t get_led_8_8_12(uint8_t x, uint8_t y, uint8_t z);


typedef void(*iterate_xy_t)(uint8_t,uint8_t);

/**
 * Iterates x, y voxels
 */
void iterate_xy(iterate_xy_t f);

typedef void(*iterate_xyz_t)(uint8_t,uint8_t,uint8_t);

/**
 * Iterates all voxels
 */
void iterate_xyz(iterate_xyz_t f);

/**
 * Sets all voxels in back buffer as black
 */
void clear_buffer(void);

extern uint16_t ticks;
extern sensors_t sensors;
extern const void *custom_data;

#define MAX_INTENSITY ((1<<GS_DEPTH)-1)

#endif // EFFECT_UTILS_H
