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

static Surface *CreateSurface(int width, int height)
{
	Surface *surface = malloc(sizeof(*surface));
	surface->tex = vita2d_create_empty_texture(width, height);
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
	if (fullscreen == 1)
		return 1;
	else
		return 0;
}

void
vid_scale(Uint32 scale_factor)
{
	/*if (screen != NULL)
		screen = SDL_SetVideoMode(gb_height*scale_factor, gb_width*scale_factor, 32, SDL_RESIZABLE|fullscreen);*/
	scale = scale_factor;
	pos_x = SCREEN_W/2 - (160/2)*scale;
	pos_y = SCREEN_H/2 - (144/2)*scale;
}

void
vid_toggle_aalias()
{
	anti_alias = (~anti_alias)&1;
}

void
vid_toggle_fullscreen()
{
	if (fullscreen == 1)
		fullscreen = 0;
	else
		fullscreen = 1;
	/*if ( (screen = SDL_SetVideoMode(gb_height*scale, gb_width*scale, 32, SDL_RESIZABLE|fullscreen)) == NULL)
		printf("SDL error %s\n", SDL_GetError());*/
}

void
vid_reset()
{
	FreeSurface(x1);
	FreeSurface(zoomS);
	FreeSurface(back);
	FreeSurface(screen);
	scale=1;
	anti_alias=0;
}

void
vid_sgb_mask()
{
	switch (scale) {
		case 1:
			temp = sgb_1;
			break;
	}

	if (temp==NULL)
		while (1) ;

	if (temp==sgb_1) {
		if (sgb_mask) {
			//SDL_FillRect(back,NULL,0);
		} else {
			//SDL_BlitSurface(sgb_buf, NULL, screen, NULL);
		}
	}
	else {
		//sgb_buf_back = _zoomSurfaceRGBA(sgb_buf, temp, 0, 0, anti_alias);
		if (sgb_mask) {
			//SDL_FillRect(back,NULL,0);
		} else {
			//SDL_BlitSurface(sgb_buf_back, NULL, screen, NULL);
		}
	}
}

static unsigned int ticks = 0;

void
vid_frame_update()
{
	/*if (gboy_mode==SGB)
		vid_sgb_mask();

	switch (scale) {
		case 1:
			temp = x1;
			break;
	}

	if (temp==NULL)
		while (1) ;
	*/
	/*SDL_Rect dstR;
	SDL_Rect scrR;
	dstR.x = gboy_mode==SGB ? 48 : 0;
	dstR.y = gboy_mode==SGB ? 40 : 0;
	dstR.x *= scale;
	dstR.y *= scale;

	scrR.w = back->w;
	scrR.h = back->h;
	scrR.x = 0;
	scrR.y = 0;*/

	vita2d_start_drawing();
	vita2d_clear_screen();

	if (temp==x1) {
		//SDL_BlitSurface(back, &scrR, screen, &dstR);
	} else {
		//back1 = _zoomSurfaceRGBA(back, temp, 0, 0, anti_alias);
		//scrR.w = back1->w;
		//scrR.h = back1->h;
		//SDL_BlitSurface(back1, &scrR, screen, &dstR);
	}

	vita2d_draw_texture_scale(back->tex, pos_x, pos_y, scale, scale);

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
		gb_height=256;
		gb_width=224;
	}

	back = CreateSurface(160, 146);

	switch (scale) {
			case 1:
				screen = CreateSurface(gb_height, gb_width);
				break;
	}

	x1 = CreateSurface(160, 144+2);
	sgb_1 = CreateSurface(256, 224);
}
