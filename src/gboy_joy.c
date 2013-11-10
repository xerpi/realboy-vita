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
Uint32 key_bitmap;
extern void vid_scale(Uint32);

long
joy_event(SDL_KeyboardEvent *key, Uint32 type)
{

	if (type == SDL_KEYDOWN) {
		switch (key->keysym.sym) {
			case SDLK_RETURN:
				key_bitmap|=RET_MASK;
				break;
			case SDLK_d:
				key_bitmap|=D_MASK;
				break;
			case SDLK_s:
				key_bitmap|=S_MASK;
				break;
			case SDLK_a:
				key_bitmap|=A_MASK;
				break;
			case SDLK_UP:
				key_bitmap|=UP_MASK;
				break;
			case SDLK_DOWN:
				key_bitmap|=DOWN_MASK;
				break;
			case SDLK_LEFT:
				key_bitmap|=LEFT_MASK;
				break;
			case SDLK_RIGHT:
				key_bitmap|=RIGHT_MASK;
				break;
			case SDLK_1:
				vid_scale(1);
				break;
			case SDLK_2:
				vid_scale(2);
				break;
			case SDLK_3:
				vid_scale(3);
				break;
			case SDLK_4:
				vid_scale(4);
				break;
			case SDLK_5:
				vid_toggle_aalias();
				break;
			case SDLK_ESCAPE:
				key_bitmap=0;
				return 1;
			case SDLK_7:
				gddb_start();
				break;
			case SDLK_6:
				vid_toggle_fullscreen();
				break;
		}
	}
	else if (type == SDL_KEYUP) {
		switch (key->keysym.sym) {
			case SDLK_UP:
				key_bitmap&=~UP_MASK;
				break;
			case SDLK_DOWN:
				key_bitmap&=~DOWN_MASK;
				break;
			case SDLK_LEFT:
				key_bitmap&=~LEFT_MASK;
				break;
			case SDLK_RIGHT:
				key_bitmap&=~RIGHT_MASK;
				break;
			case SDLK_a:
				key_bitmap&=~A_MASK;
				break;
			case SDLK_s:
				key_bitmap&=~S_MASK;
				break;
			case SDLK_d:
				key_bitmap&=~D_MASK;
				break;
			case SDLK_RETURN:
				key_bitmap&=~RET_MASK;
				break;
		}
	}

	return 0;
}
