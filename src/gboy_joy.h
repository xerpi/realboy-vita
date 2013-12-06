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
long key_bitmap;
extern SDLKey key_binds_SDL[4]; // Array of SDL keybindings. In order: A, B, Start, Select.
extern void vid_scale(Uint32);
extern int vid_is_fullscreen();
extern void vid_toggle_aalias();
extern void vid_toggle_fullscreen();
extern int getch();
extern void frame_speeddown();
extern void frame_speedup();
extern void joy_chbinds();
extern void gddb_start();
