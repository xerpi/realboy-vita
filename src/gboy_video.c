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

/* Locally-global variables */
static SDL_Surface *back1;
static SDL_Surface *temp=NULL;

/* Global/Exported variables */
Uint32 scale=1;
Uint32 anti_alias=0;
SDL_Surface *zoomS;
SDL_Surface *x1;
SDL_Surface *x2;
SDL_Surface *x3;
SDL_Surface *x4;
Uint32 fullscreen;

/* Imported/External variables */
extern SDL_Surface *_zoomSurfaceRGBA(SDL_Surface *, SDL_Surface *, int, int, int);
extern SDL_Surface *back;

void
vid_scale(Uint32 scale_factor)
{
	screen = SDL_SetVideoMode(160*scale_factor, 144*scale_factor, 32, SDL_RESIZABLE|fullscreen);
	scale = scale_factor;
}

void
vid_toggle_aalias()
{
	anti_alias = (~anti_alias)&1;
}

void
vid_toggle_fullscreen()
{
	if (fullscreen==SDL_FULLSCREEN)
		fullscreen=0;
	else
		fullscreen=SDL_FULLSCREEN;
	if ( (screen = SDL_SetVideoMode(160*scale, 144*scale, 32, SDL_RESIZABLE|fullscreen)) == NULL)
		printf("SDL error %s\n", SDL_GetError());
}

void
vid_reset()
{
	SDL_FreeSurface(x1);
	SDL_FreeSurface(x2);
	SDL_FreeSurface(x3);
	SDL_FreeSurface(x4);
	SDL_FreeSurface(zoomS);
	SDL_FreeSurface(back);
	SDL_FreeSurface(screen);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	scale=1;
	anti_alias=0;
}

void
vid_frame_update()
{

	switch (scale) {
		case 1:
			temp = x1;
			break;
		case 2:
			temp = x2;
			break;
		case 3:
			temp = x3;
			break;
		case 4:
			temp = x4;
			break;
	}

	if (temp==NULL)
		while (1) ;

	if (temp==x1)
		SDL_BlitSurface(back, NULL, screen, NULL);
	else {
		back1 = _zoomSurfaceRGBA(back, temp, 0, 0, anti_alias);
		SDL_BlitSurface(back1, NULL, screen, NULL);
	}

  SDL_Flip(screen);
}

void
vid_start()
{
	int flags, i;
	unsigned int cur_col;

	flags = 0; // XXX

	/* Initialize Game Boy or Game Boy Color palette */
	if (gboy_mode==0)
	{
		pal_grey[0]=0xffffff;
		pal_grey[1]=0x917d5e;
		pal_grey[2]=0x635030;
		pal_grey[3]=0x211a10;
	}
	else
	{
		for (i=0; i<32768; i++)
		{
			cur_col = (((i&0x1f)<<3)<<16) | ((((i>>5)&0x1f)<<3)<<8) | (((i>>10)&0x1f)<<3);
			pal_color[i] = cur_col;
		}
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	back = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 146, 32, 0, 0, 0, 0);
	screen = SDL_SetVideoMode(160, 144, 32, SDL_RESIZABLE);
	switch (scale) {
			case 2:
				screen = SDL_SetVideoMode(160*2, 144*2, 32,SDL_RESIZABLE|fullscreen);
				break;
			case 3:
				screen = SDL_SetVideoMode(160*3, 144*3, 32,SDL_RESIZABLE|fullscreen);
				break;
			case 4:
				screen = SDL_SetVideoMode(160*4, 144*4, 32,SDL_RESIZABLE|fullscreen);
				break;
	}

	SDL_ShowCursor(0);
	SDL_WM_SetCaption(gb_cart.cart_name, gb_cart.cart_name);
	x1 = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144+2 , 32, 0, 0, 0, 0);
	x2 = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 288+3 , 32, 0, 0, 0, 0);
	x3 = SDL_CreateRGBSurface(SDL_SWSURFACE, 480, 432+5 , 32, 0, 0, 0, 0);
	x4 = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 576+7 , 32, 0, 0, 0, 0);
}
