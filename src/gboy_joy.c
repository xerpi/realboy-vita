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
#include "gboy_joy.h"

long
joy_event(SDL_KeyboardEvent *key, Uint32 type)
{
	if (type == SDL_KEYDOWN) {
		if (key->keysym.sym == key_binds_SDL[0])
				key_bitmap|=D_MASK;
		else if (key->keysym.sym == key_binds_SDL[1])
				key_bitmap|=S_MASK;
		else if (key->keysym.sym == key_binds_SDL[2])
				key_bitmap|=RET_MASK;
		else if (key->keysym.sym == key_binds_SDL[3])
				key_bitmap|=A_MASK;
		else {
			switch (key->keysym.sym) {
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
				case SDLK_6:
					vid_toggle_fullscreen();
					break;
				case SDLK_7:
					if (vid_is_fullscreen()) {
						vid_toggle_fullscreen();
						printf("\n\n========================================================================\n");
						printf("FULLSCREEN HAS BEEN DISABLED TO STAR GDDB DEBUGGER\n");
						printf("Press any key to start GDDB\n");
						getch();
						gddb_start();
					}
					else
						gddb_start();
					break;
				case SDLK_8:
					if (vid_is_fullscreen()) {
						vid_toggle_fullscreen();
						printf("\n\n========================================================================\n");
						printf("FULLSCREEN WILL BE RESTORED WHEN KEY BINDING CONFIGURATION IS FINISHED\n");
						printf("Press any key to begin configuration\n");
						getch();
						joy_chbinds();
						vid_toggle_fullscreen();
					}
					else
						joy_chbinds();
					break;
				case SDLK_ESCAPE:
					key_bitmap=0;
					return 1;
				case SDLK_PERIOD:
					frame_speedup();
					break;
				default:
					;
			}
		}
	}
	else if (type == SDL_KEYUP) {
		if (key->keysym.sym == key_binds_SDL[0])
				key_bitmap&=~D_MASK;
		else if (key->keysym.sym == key_binds_SDL[1])
				key_bitmap&=~S_MASK;
		else if (key->keysym.sym == key_binds_SDL[2])
				key_bitmap&=~RET_MASK;
		else if (key->keysym.sym == key_binds_SDL[3])
				key_bitmap&=~A_MASK;
		else {
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
				case SDLK_PERIOD:
					frame_speeddown();
					break;
				default:
					;
			}
		}
	}

	return 0;
}
