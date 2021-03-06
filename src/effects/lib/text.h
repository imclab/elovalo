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

#include "../../common/pgmspace.h"
#include "font8x8.h"

typedef void(*render_t)(uint8_t x, uint8_t y);

/**
 * Scrolls given text using given callback
 */
void scroll_text(const struct glyph_buf *text_p, enum mem_type mem, int16_t offset, render_t f);

/**
 * Renders character (index from font8x8_basic) using given callback
 */
void render_character(const struct glyph *glyph_p, int16_t offset, render_t f);

/**
 * Render helpers
 * */

void render_yz(uint8_t x, uint8_t y);
void render_xy(uint8_t x, uint8_t y);
