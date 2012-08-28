/**
 * Led cube effects
 *
 * Not integrated to the main code, yet.
 */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "pgmspace.h"
#include "env.h"
#include "effects.h"
#include "effect_utils.h"
#include "text.h"

// Private functions
static void init_all_on(void);
static void init_stairs(void);
static void init_game_of_life(void);
static void init_brownian(void);
static void init_worm(void);

static void effect_scroll_text(void);
static void effect_character(void);
static void effect_game_of_life(void);
static void effect_heart(void);
static void effect_brownian(void);
static void effect_sine(void);
static void effect_wave(void);
static void effect_sphere(void);
static void effect_worm(void);
static void effect_constant(void);
static void effect_layers_tester(void);

/* Some important notes about adding new effects:
 *
 * a) Add strings as PROGMEM entries like the ones below.
 *
 * b) Add entry to effects[] array
 */

PROGMEM const char s_stairs[]       = "stairs";
PROGMEM const char s_game_of_life[] = "game_of_life";
PROGMEM const char s_heart[]        = "heart";
PROGMEM const char s_brownian[]     = "brownian";
PROGMEM const char s_sine[]         = "sine";
PROGMEM const char s_wave[]         = "wave";
PROGMEM const char s_sphere[]       = "sphere";
PROGMEM const char s_worm[]         = "worm";
PROGMEM const char s_const[]        = "const";
PROGMEM const char s_layers[]       = "layers";
PROGMEM const char s_all_on[]       = "all_on";
PROGMEM const char s_character[]    = "character";
PROGMEM const char s_scroll_text[]  = "scroll_text";

const effect_t effects[] PROGMEM = {
	{ s_stairs, &init_stairs, NULL, 100, NO_FLIP},
	{ s_game_of_life, &init_game_of_life, &effect_game_of_life, 2000, FLIP},
	{ s_heart, NULL, &effect_heart, 100, FLIP},
	{ s_brownian, &init_brownian, &effect_brownian, 100, NO_FLIP },
	{ s_sine, NULL, &effect_sine, 6000, FLIP },
	{ s_wave, NULL, &effect_wave, 600, FLIP },
	{ s_sphere, NULL, &effect_sphere, 100, FLIP },
	{ s_worm, &init_worm, &effect_worm, 600, NO_FLIP },
	{ s_const, NULL, &effect_constant, 100, FLIP },
	{ s_layers, NULL, &effect_layers_tester, 3000, FLIP },
	{ s_all_on, &init_all_on, NULL, 1000, FLIP },
	{ s_character, NULL, &effect_character, 100, NO_FLIP },
	{ s_scroll_text, NULL, &effect_scroll_text, 100, FLIP }
};

const uint8_t effects_len = sizeof(effects) / sizeof(effect_t);

/**
 * Text scroller
 */
void effect_scroll_text(void)
{
	clear_buffer();
	scroll_text("elovalo", 3, (1<<GS_DEPTH) - 1, -(ticks >> 2));
}

/**
 * Character tester.
 */
void effect_character(void)
{
	render_character('c', 0, (1<<GS_DEPTH) - 1, 0);
}

/**
 * Stairs tester.
 */
static void init_stairs(void)
{
	for(uint8_t i = 0; i < 8; i++) {
		set_row(i, i, 0, 7, MAX_INTENSITY);
	}
}

/**
 * Conway's game of life in 3D
 */
static uint8_t get_amount_of_neighbours(uint8_t x, uint8_t y, uint8_t z);
static void set_gol_intensity(uint8_t x, uint8_t y, uint8_t z);
static void init_game_of_life(void)
{
	// TODO: might want to use some other seed. using heart for testing
	effect_heart();
}
static void effect_game_of_life(void)
{
	iterate_3d(set_gol_intensity);
}
static void set_gol_intensity(uint8_t x, uint8_t y, uint8_t z) {
	uint8_t neighbours = get_amount_of_neighbours((int8_t)x, (int8_t)y, (int8_t)z);

	if(neighbours >= 9 && neighbours <= 17) set_led(x, y, z, MAX_INTENSITY);
	else set_led(x, y, z, 0);
}
static uint8_t get_amount_of_neighbours(uint8_t x, uint8_t y, uint8_t z) {
	uint8_t ret = 0;

	for(int8_t cx = -1; cx <= 1; cx++)
		for(int8_t cy = -1; cy <= 1; cy++)
			for(int8_t cz = -1; cz <= 1; cz++)
				ret += get_led_wrap(x + cx, y + cy, z + cz) > 0;

	return ret;
}

/**
 * Heart effect. Supposed to beat according to some input.
 */
static void heart(uint8_t x, uint16_t intensity);
static void effect_heart(void)
{
	heart(1, (float)MAX_INTENSITY / 100);
	heart(2, (float)MAX_INTENSITY / 25);
	heart(3, MAX_INTENSITY);
	heart(4, MAX_INTENSITY);
	heart(5, (float)MAX_INTENSITY / 25);
	heart(6, (float)MAX_INTENSITY / 100);
}
static void heart(uint8_t x, uint16_t intensity) {
	set_row(x, 0, 1, 2, intensity);
	set_row(x, 0, 5, 6, intensity);
	set_row(x, 1, 0, 7, intensity);
	set_row(x, 2, 0, 7, intensity);
	set_row(x, 3, 0, 7, intensity);
	set_row(x, 4, 1, 6, intensity);
	set_row(x, 5, 2, 5, intensity);
	set_row(x, 6, 3, 4, intensity);
}

/**
 * Brownian particle. Starts near center and then accumulates.
 */
uint8_t brown_x;
uint8_t brown_y;
uint8_t brown_z;
static void init_brownian(void)
{
	brown_x = (uint8_t)(LEDS_X / 2);
	brown_y = (uint8_t)(LEDS_Y / 2);
	brown_z = (uint8_t)(LEDS_Z / 2);

	clear_buffer();
}
static void effect_brownian(void)
{
	set_led(brown_x, brown_y, brown_z, MAX_INTENSITY);

	brown_x = clamp(brown_x + randint(-2, 2), 0, LEDS_X - 1);
	brown_y = clamp(brown_y + randint(-2, 2), 0, LEDS_Y - 1);
	brown_z = clamp(brown_z + randint(-2, 2), 0, LEDS_Z - 1);
}

/**
 * Plot 2-dimensional sine waves which are moving. The parameters are
 * not tuned, this is just taken from my head.
 */
TWOD(effect_sine)
{
	const float sine_scaler = (float)MAX_2D_PLOT_INTENSITY/4;

	return sine_scaler * (2 + sin((float)x/2+(float)ticks/15) + sin((float)y/2+(float)ticks/30));
}

/**
 * Plot 1-dimensional sine waves.
 */
TWOD(effect_wave)
{
	const float sine_scaler = (float)MAX_2D_PLOT_INTENSITY / 4;

	return sine_scaler * (2 + sin((float)x * 50 + (float)ticks / 15));
}

/**
 * Plot sphere.
 */
static void effect_sphere(void)
{
	const float a = -3.5 - 3 * (float)(ticks % 26 - 13) / 13;
	const float rsq_max = 16;
	const float rsq_min = 8;
	const float fac = 0.2;

	clear_buffer();

	for (float x = a; x < LEDS_X + a; x++) {
		for (float y = a; y < LEDS_Y + a; y++) {
			for (float z = a; z < LEDS_Z + a; z++) {
				float sq = x * x + y * y + z * z;

				if(rsq_min * fac < sq && sq < rsq_max * fac) {
					set_led(x - a, y - a, z - a, MAX_INTENSITY);
				}
			}
		}
	}
}

/**
 * Plot worm.
 */
uint16_t worm_pos[3];
uint16_t worm_dir;
int worm_speed;
static void init_worm(void)
{
	worm_pos[0] = 4;
	worm_pos[1] = 4;
	worm_pos[2] = 4;
	worm_dir = 0;
	worm_speed = 1;

	clear_buffer();
}
static void effect_worm(void)
{
	set_led(worm_pos[0], worm_pos[1], worm_pos[2], MAX_INTENSITY);

	// Relies on the fact that it's a cube
	// Not entirely fool proof but probably good enough
	int new_pos = worm_pos[worm_dir] + worm_speed;
	if(new_pos < 0 || new_pos >= LEDS_X || randint(0, 10) > 7) {
		worm_dir = worm_dir + 1 > 2? 0: worm_dir + 1;
		worm_speed = -worm_speed;
	}

	worm_pos[worm_dir] += worm_speed;
}

/**
 * Constant plot. Helps to spot errors in drawing code. Replace this
 * with something more useful in the future.
 */
TWOD(effect_constant)
{
	return 10000;
}

/**
 * Simple test function which draws voxel layers.
 */
static void effect_layers_tester(void)
{
	clear_buffer();
	uint8_t z = ((ticks >> 7) % LEDS_Z);
	const uint16_t val = sensors.debug_value > MAX_INTENSITY ?
		MAX_INTENSITY : sensors.debug_value;

	for (uint8_t x=0; x<LEDS_X; x++) {
		for (uint8_t y=0; y<LEDS_Y; y++) {
			set_led(x, y, z, val);
		}
	}
}

/**
 * Simple effect: all voxel on
 */
static void init_all_on(void)
{
	for (float x=0; x < LEDS_X; x++) {
		for (float y=0; y < LEDS_Y; y++) {
			for (float z=0; z < LEDS_Z; z++) {
				set_led(x,y,z,MAX_INTENSITY);
			}
		}
	}
}
