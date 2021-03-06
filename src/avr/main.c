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

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <stdlib.h>
#include "pinMacros.h"
#include "init.h"
#include "tlc5940.h"
#include "hcsr04.h"
#include "adc.h"
#include "serial_target.h"
#include "clock.h"
#include "configuration.h"
#include "powersave.h"
#include "main.h"
#include "zcl_skeleton.h"
#include "../common/pgmspace.h"
#include "../common/cube.h"
#include "../common/effects.h"
#include "../common/playlists.h"

uint8_t mode = MODE_IDLE; // Starting with no operation on.
const effect_t *effect; // Current effect. Note: points to PGM

uint16_t effect_length; // Length of the current effect. Used for playlist
static uint16_t next_draw_at = 0; // Used for FPS limiting

// It might be nice to use this for single effect too (set via serial).
uint8_t active_effect; // Index of the active effect. Used for playlist
uint8_t active_playlist; // Index of the active playlist

// Variables used only in simulation mode
#ifdef SIMU
uint8_t simulation_mode __attribute__ ((section (".noinit")));
uint8_t simulation_effect __attribute__ ((section (".noinit")));
#endif

struct {
	bool mode:1;
	bool playlist:1;
	bool effect:1;
	bool text:1;
} modified = {true,true,true,true};

// Private functions
static void init_playlist(void);
static void next_effect();
static void pick_startup_mode(void);

int main() {
	cli();

	wdt_disable(); // To make sure nothing weird happens
	init_tlc5940();
	init_spi();
	init_ps();

	init_blank_timer();
	init_effect_timer();
	
	init_playlist();
	
	initUSART();
	sei();

	hcsr04_start_continuous_meas();
	adc_start();

	serial_boot_report();
	
	// Select correct startup mode
	pick_startup_mode();

	while(1) {
		/* Serial processing is implementation specific and defined in
		 * serial_common.c */
		process_serial();

		switch (mode) {
		case MODE_SLEEP:
			// Fall through to MODE_IDLE
		case MODE_IDLE:
			// No operation
			sleep_if_no_traffic();
			break;
		case MODE_PLAYLIST:
			ticks = centisecs();
			if (ticks > effect_length) {
				next_effect();
				init_current_effect();
			}

			// no need to break!
			// fall to MODE_EFFECT on purpose
		case MODE_EFFECT:
			// If a buffer is not yet flipped, wait interrupts
			if (flags.may_flip) {
				sleep_if_no_traffic();
				break;
			}

			// Update clock
			ticks = centisecs();
	
			/* Go back to serial handler if drawing time
			 * is reached. By doing this we avoid serial
			 * port slowdown when FPS is low */
			if (ticks < next_draw_at ) {
				sleep_if_no_traffic();
				break;
			}

			/* Restart effect if maximum ticks is
			 * reached. This may result a glitch but is
			 * better than the effect to stop. */
			if (ticks == ~0) {
				init_current_effect();
				ticks = 0;
			}	

			// Update sensor values
			sensors.distance1 = hcsr04_get_distance_in_cm();
			sensors.distance2 = hcsr04_get_distance_in_cm(); //TODO: use separate sensor
			sensors.ambient_light = adc_get(0) >> 2;
			sensors.sound_pressure_level = adc_get(1) >> 2;

			// Do the actual drawing
			draw_t draw = (draw_t)pgm_get(effect->draw,word);
			if (draw != NULL) {
				draw();
				allow_flipping(true);
			}

			// Update time when next drawing is allowed
			next_draw_at = ticks + pgm_get(effect->minimum_ticks,byte);

			break;
		}
	}

	return 0;
}

#if defined SIMU
static void pick_startup_mode(void)
{
	// Start normally
	cube_start(0);

	mode = simulation_mode;
	switch (mode) {
	case MODE_EFFECT:
		effect = effects + simulation_effect;
		init_current_effect();
		break;
	case MODE_PLAYLIST:
		change_playlist(0);
		break;
	default:
		mode = MODE_IDLE;
	}
}
#elif defined AVR_ZCL
static void pick_startup_mode(void)
{
	// Reading configrutaion etc. from non-volatile memory
	init_zcl();
	uint8_t start_mode = read_mode();

	// Actual mode selection
	if (start_mode != MODE_SLEEP) {
		cube_start(0);
		// cube_start() does implicit modification to 'mode'.
	}

	mode = start_mode;
	use_stored_effect();
	use_stored_playlist();
}
#elif defined AVR_ELO
static void pick_startup_mode(void)
{
	cube_start(0);

	// Quick fix to start in kiosk mode
	change_playlist(0);
}
#else
#error Unknown variant
#endif

static void init_playlist(void) {
	select_playlist_item(0);
}

static void next_effect() {
	if (active_effect+1 == master_playlist_len ||
	    active_effect+1 == pgm_get(playlists[active_playlist+1],byte)) {
		// End reached. Go to the first item of the playlist.
		select_playlist_item(pgm_get(playlists[active_playlist],byte));
	} else {
		// Advance to the next item in playlist
		select_playlist_item(active_effect + 1);
	}
}

void select_playlist_item(uint8_t index) {
	active_effect = index;
	const playlistitem_t *item = master_playlist + index;
	uint8_t e_id = pgm_get(item->id,byte);
	effect = effects + e_id;
	effect_length = pgm_get(item->length,word);
	custom_data = (void*)pgm_get(item->data,word);
}

void init_current_effect(void) {
	// Disable flipping until first frame is drawn
	allow_flipping(false);

	/* Restore front and back buffer pointers to point to
	 * different locations */
	gs_restore_bufs();

	// Set up rng
	srand_from_clock();

	// Run initializer
	init_t init = (init_t)pgm_get(effect->init, word);
	if (init != NULL) init();
	gs_buf_swap();
	
	/* If NO_FLIP, we "broke" flipping if required by pointing
	 * both buffers to the same location */
	if (pgm_get(effect->flip_buffers, byte) == NO_FLIP) {
		gs_buf_back = gs_buf_front;
	}
	
	// Restart tick counter and FPS limiter
	reset_time();
	next_draw_at = 0;
}

uint8_t change_current_effect(uint8_t i) {
	if (i >= effects_len) { return 1; }

	// Change mode and pick correct effect from the array.
	mode = MODE_EFFECT;
	effect = effects + i;
	custom_data = NULL; // Used in playlists only

	// Prepare running of the new effect
	init_current_effect();

	return 0;
}

void use_stored_effect(void)
{
	if (mode != MODE_EFFECT) return;
	if (!(modified.effect || modified.mode ||
	      (modified.text && pgm_get(effect->dynamic_text, byte))))
	{
		return;
	}

	uint8_t new_effect = read_effect();
	// Avoid dangling pointers and extra initialization
      	if (new_effect >= effects_len) new_effect = 0;

	effect = effects + new_effect;
	custom_data = NULL; // Used in playlists only
	init_current_effect();
}

void use_stored_playlist(void)
{
	if (mode != MODE_PLAYLIST) return;
	if (!(modified.playlist || modified.mode)) return;

	uint8_t new_playlist = read_playlist();
	// Avoid dangling pointers and extra initialization
	if (new_playlist >= playlists_len) new_playlist = 0;

	// Activate
	active_playlist = new_playlist;
	select_playlist_item(pgm_get(playlists[new_playlist],byte));
	init_current_effect();
}

uint8_t change_playlist(uint8_t i) {
	if (i >= playlists_len) { return 1; }

	active_playlist = i;

	// Change mode and run init
	mode = MODE_PLAYLIST;
	select_playlist_item(pgm_get(playlists[i],byte));
	init_current_effect();

	return 0;
}

uint8_t get_mode(void) {
	return mode;
}

void set_mode(uint8_t new_mode) {
	if (mode == new_mode) return;
	store_mode(new_mode);
	modified.mode = true;

	if (mode == MODE_SLEEP) {
		cube_start(0);
	} else if (new_mode == MODE_SLEEP) {
		cube_shutdown(0);
	}

	mode = new_mode;
}

void reset_modified_state(void)
{
	modified.effect = false;
	modified.playlist = false;
	modified.mode = false;
	modified.text = false;
}

void mark_playlist_modified(void) {
	modified.playlist = true;
}

void mark_effect_modified(void) {
	modified.effect = true;
}

void mark_text_modified(void) {
	modified.text = true;
}

//If an interrupt happens and there isn't an interrupt handler, we go here!
ISR(BADISR_vect)
{
	pin_high(DEBUG_LED); //Give us an indication about an error condition...
	while(1){
		}
}
