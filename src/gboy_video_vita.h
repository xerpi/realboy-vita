/* RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
 * Copyright (C) 2013 Sergio Andrés Gómez del Real
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/* Locally-global variables */
static Surface *back1 = NULL;
static Surface *temp=NULL;

/* Global/Exported variables */
Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
Uint32 pal_grey[4]; // 32-bit palette for monochrome Game Boy
Uint32 pal_color[32*32*32]; // 32-bit palette for Color Game Boy
Uint32 gb_height;
Uint32 gb_width;
Uint32 scale=1;
Uint32 anti_alias=0;
Surface *screen = NULL;
Surface *back = NULL;
Surface *sgb_buf = NULL;
Surface *sgb_buf_back = NULL;
Surface *zoomS = NULL;
Surface *x1 = NULL;
Surface *sgb_1 = NULL;
Uint32 fullscreen;

/* Imported/External variables */
extern Uint32 gboy_mode; // Game Boy/Color Game Boy mode
extern Surface *_zoomSurfaceRGBA(Surface *, Surface *, int, int, int);
extern Surface *back;
extern long sgb_mask;

