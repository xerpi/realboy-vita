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
static SDL_Surface *back1;
static SDL_Surface *temp=NULL;

/* Global/Exported variables */
Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
Uint32 pal_grey[4]; // 32-bit palette for monochrome Game Boy
Uint32 pal_color[32*32*32]; // 32-bit palette for Color Game Boy
Uint32 gb_height;
Uint32 gb_width;
Uint32 scale=1;
Uint32 anti_alias=0;
SDL_Surface *screen;
SDL_Surface *back;
SDL_Surface *sgb_buf;
SDL_Surface *sgb_buf_back;
SDL_Surface *zoomS;
SDL_Surface *x1;
SDL_Surface *x2;
SDL_Surface *x3;
SDL_Surface *x4;
SDL_Surface *sgb_1;
SDL_Surface *sgb_2;
SDL_Surface *sgb_3;
SDL_Surface *sgb_4;
Uint32 fullscreen;

/* Imported/External variables */
extern Uint32 gboy_mode; // Game Boy/Color Game Boy mode
extern SDL_Surface *_zoomSurfaceRGBA(SDL_Surface *, SDL_Surface *, int, int, int);
extern SDL_Surface *back;
extern long sgb_mask;

