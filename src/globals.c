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

#include <SDL/SDL.h>
#define MAX_STRS 8

/* Misc globals */
char inp_buf[512];
char *cmd_ptrs[MAX_STRS+2];
FILE *boot_file=NULL;
FILE *rom_file=NULL;
char *home_path=NULL;
char *file_path=NULL;
Uint8 addr_sp[0x10000];
long chg_gam=0;
long addr_sp_ptrs[16] = { 0 };
