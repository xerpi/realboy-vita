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
Uint32 gb_height;
Uint32 gb_width;
Uint32 scale=1;
Uint32 anti_alias=0;
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
extern SDL_Surface *_zoomSurfaceRGBA(SDL_Surface *, SDL_Surface *, int, int, int);
extern SDL_Surface *back;
extern long sgb_mask;

void
vid_set_fullscreen()
{
	fullscreen = SDL_FULLSCREEN;
}

void
vid_no_fullscreen()
{
	fullscreen = 0;
}

int
vid_is_fullscreen()
{
	if (fullscreen==SDL_FULLSCREEN)
		return 1;
	else
		return 0;
}

void
vid_scale(Uint32 scale_factor)
{
	if (screen != NULL)
		screen = SDL_SetVideoMode(gb_height*scale_factor, gb_width*scale_factor, 32, SDL_RESIZABLE|fullscreen);
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
	if ( (screen = SDL_SetVideoMode(gb_height*scale, gb_width*scale, 32, SDL_RESIZABLE|fullscreen)) == NULL)
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
vid_sgb_mask()
{

	switch (scale) {
		case 1:
			temp = sgb_1;
			break;
		case 2:
			temp = sgb_2;
			break;
		case 3:
			temp = sgb_3;
			break;
		case 4:
			temp = sgb_4;
			break;
	}

	if (temp==NULL)
		while (1) ;
	
	if (temp==sgb_1) {
		if (sgb_mask) 
			SDL_FillRect(back,NULL,0);
		else
			SDL_BlitSurface(sgb_buf, NULL, screen, NULL);
	}
	else {
		sgb_buf_back = _zoomSurfaceRGBA(sgb_buf, temp, 0, 0, anti_alias);
		if (sgb_mask) 
			SDL_FillRect(back,NULL,0);
		else
			SDL_BlitSurface(sgb_buf_back, NULL, screen, NULL);
	}
}

void
vid_frame_update()
{
	if (gboy_mode==SGB)
		vid_sgb_mask();

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

	SDL_Rect dstR;
	SDL_Rect scrR;
	dstR.x = gboy_mode==SGB ? 48 : 0;
	dstR.y = gboy_mode==SGB ? 40 : 0;
	dstR.x *= scale;
	dstR.y *= scale;
	
	scrR.w = back->w;
	scrR.h = back->h;
	scrR.x = 0;
	scrR.y = 0;
	if (temp==x1)
		SDL_BlitSurface(back, &scrR, screen, &dstR);
	else {
		back1 = _zoomSurfaceRGBA(back, temp, 0, 0, anti_alias);
		scrR.w = back1->w;
		scrR.h = back1->h;
		SDL_BlitSurface(back1, &scrR, screen, &dstR);
	}

//	while (*(long *)0x671840 != 2)
//		;
	SDL_Flip(screen);
}

void
vid_start()
{
	int flags, i;
	unsigned int cur_col;

	flags = 0; // XXX

	/* Initialize Game Boy or Game Boy Color palette */
	if (gboy_mode==DMG)
	{
		pal_grey[0]=0xffffff;
		pal_grey[1]=0x917d5e;
		pal_grey[2]=0x635030;
		pal_grey[3]=0x211a10;
		gb_height=160;
		gb_width=144;
	}
	else if (gboy_mode==CGB)
	{
		for (i=0; i<32768; i++)
		{
			cur_col = (((i&0x1f)<<3)<<16) | ((((i>>5)&0x1f)<<3)<<8) | (((i>>10)&0x1f)<<3);
			pal_color[i] = cur_col;
		}
		gb_height=160;
		gb_width=144;
	}
	else {
		pal_grey[0]=0xffffff;
		pal_grey[1]=0x917d5e;
		pal_grey[2]=0x635030;
		pal_grey[3]=0x211a10;
		for(i=0;i<4;i++) {
			pal_sgb[i][0]=pal_grey[0];
			pal_sgb[i][1]=pal_grey[1];
			pal_sgb[i][2]=pal_grey[2];
			pal_sgb[i][3]=pal_grey[3];
		}	
		sgb_buf = SDL_CreateRGBSurface(SDL_SWSURFACE, 256, 226, 32, 0, 0, 0, 0);
		gb_height=256;
		gb_width=224;
	}

	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	back = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 146, 32, 0, 0, 0, 0);

	switch (scale) {
			case 1:
				screen = SDL_SetVideoMode(gb_height, gb_width, 32, SDL_RESIZABLE|fullscreen);
				break;
			case 2:
				screen = SDL_SetVideoMode(gb_height*2, gb_width*2, 32, SDL_RESIZABLE|fullscreen);
				break;
			case 3:
				screen = SDL_SetVideoMode(gb_height*3, gb_width*3, 32, SDL_RESIZABLE|fullscreen);
				break;
			case 4:
				screen = SDL_SetVideoMode(gb_height*4, gb_width*4, 32, SDL_RESIZABLE|fullscreen);
				break;
	}

	SDL_ShowCursor(0);
	SDL_WM_SetCaption(gb_cart.cart_name, gb_cart.cart_name);
	x1 = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144+2 , 32, 0, 0, 0, 0);
	x2 = SDL_CreateRGBSurface(SDL_SWSURFACE, 160*2, 144*2+3 , 32, 0, 0, 0, 0);
	x3 = SDL_CreateRGBSurface(SDL_SWSURFACE, 160*3, 144*3+5 , 32, 0, 0, 0, 0);
	x4 = SDL_CreateRGBSurface(SDL_SWSURFACE, 160*4, 144*4+7 , 32, 0, 0, 0, 0);
	sgb_1 = SDL_CreateRGBSurface(SDL_SWSURFACE, 256, 224 , 32, 0, 0, 0, 0);
	sgb_2 = SDL_CreateRGBSurface(SDL_SWSURFACE, 256*2, 224*2 , 32, 0, 0, 0, 0);
	sgb_3 = SDL_CreateRGBSurface(SDL_SWSURFACE, 256*3, 224*3 , 32, 0, 0, 0, 0);
	sgb_4 = SDL_CreateRGBSurface(SDL_SWSURFACE, 256*4, 224*4 , 32, 0, 0, 0, 0);
}
