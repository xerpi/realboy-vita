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
#define A_KEY 0
#define B_KEY 1
#define START_KEY 2
#define SELECT_KEY 3
#define ALL_KEYS 4

extern int getch();

char *key_binds_errs[2] = { "Conflicting Key Binding", "Invalid Key Binding" };
char *key_binds_strs[4] = { "A", "B", "START", "SELECT" };
char key_binds_ascii[4] = { 'd', 's', '\n', 'a' }; // Array of ASCII keybindings. In order: A, B, Start, Select.
SDLKey key_binds_SDL[4] = { SDLK_d, SDLK_s, SDLK_RETURN, SDLK_a }; // Array of SDL keybindings. In order: A, B, Start, Select.
