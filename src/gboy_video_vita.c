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

#include "gboy.h"
#include "gboy_video_vita.h"

static int pos_x = 0;
static int pos_y = 0;
static float fullscreen_scale_y = 0.0f;
static float fullscreen_scale_x = 0.0f;

static Surface *CreateSurface(int width, int height)
{
	Surface *surface = malloc(sizeof(*surface));
	surface->tex = vita2d_create_empty_texture_format(width, height, SCE_GXM_TEXTURE_FORMAT_X8U8U8U8_1RGB);
	surface->pixels = vita2d_texture_get_datap(surface->tex);
	surface->x = 0;
	surface->y = 0;
	surface->w = width;
	surface->h = height;
	surface->pitch = width*4;
	return surface;
}

static void FreeSurface(Surface *surface)
{
	if (surface) {
		vita2d_free_texture(surface->tex);
		free(surface);
	}
}

void
vid_set_fullscreen()
{
	fullscreen = 1;
}

void
vid_no_fullscreen()
{
	fullscreen = 0;
}

int
vid_is_fullscreen()
{
	return fullscreen;
}

void
vid_scale(Uint32 scale_factor)
{
	scale = scale_factor;
	pos_x = SCREEN_W/2 - (gb_height/2)*scale;
	pos_y = SCREEN_H/2 - (gb_width/2)*scale;
}

void
vid_toggle_aalias()
{
	anti_alias = (~anti_alias)&1;
}

void
vid_toggle_fullscreen()
{
	fullscreen ^= 1;
}

void
vid_reset()
{
	FreeSurface(back);
	FreeSurface(sgb_buf);
	back = NULL;
	sgb_buf = NULL;
}

void
vid_sgb_mask()
{
}

void
vid_frame_update()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	if (gboy_mode != SGB || !sgb_mask) {
		if (fullscreen) {
			vita2d_draw_texture_part_scale(back->tex,
				0, 0,
				0, 0,
				/* w and h are swapped */
				gb_height, gb_width,
				fullscreen_scale_x, fullscreen_scale_y);

		} else {
			vita2d_draw_texture_part_scale(back->tex,
				pos_x, pos_y,
				0, 0,
				/* w and h are swapped */
				gb_height, gb_width,
				scale, scale);
		}
	}

	vita2d_end_drawing();
	vita2d_swap_buffers();
}

void
vid_start()
{
	int flags, i;
	unsigned int cur_col;

	flags = 0; // XXX

	/* Initialize Game Boy or Game Boy Color palette */
	if (gboy_mode==DMG) {
		pal_grey[0]=0xffffff;
		pal_grey[1]=0x917d5e;
		pal_grey[2]=0x635030;
		pal_grey[3]=0x211a10;
		gb_height=160;
		gb_width=144;
	} else if (gboy_mode==CGB) {
		for (i = 0; i < 32768; i++) {
			cur_col = (((i&0x1f)<<3)<<16) | ((((i>>5)&0x1f)<<3)<<8) | (((i>>10)&0x1f)<<3);
			pal_color[i] = cur_col;
		}
		gb_height=160;
		gb_width=144;
	} else {
		pal_grey[0]=0xffffff;
		pal_grey[1]=0x917d5e;
		pal_grey[2]=0x635030;
		pal_grey[3]=0x211a10;
		for (i = 0; i < 4; i++) {
			pal_sgb[i][0]=pal_grey[0];
			pal_sgb[i][1]=pal_grey[1];
			pal_sgb[i][2]=pal_grey[2];
			pal_sgb[i][3]=pal_grey[3];
		}
		sgb_buf = CreateSurface(256, 226);
		/* Don't draw the Super GameBoy Border */
		// gb_height=256;
		// gb_width=224;
		gb_height=160;
		gb_width=144;
	}

	back = CreateSurface(160, 146);

	pos_x = SCREEN_W/2 - (gb_height/2)*scale;
	pos_y = SCREEN_H/2 - (gb_width/2)*scale;

	fullscreen_scale_x = SCREEN_W/(float)gb_height;
	fullscreen_scale_y = SCREEN_H/(float)gb_width;
}
