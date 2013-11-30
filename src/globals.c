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

/* CLI-related globals */
char inp_buf[512];
char *ptr_dup; // pointer to duplicated command for parsing
char *cmd_ptrs[MAX_STRS+2];

char *ram_sz_vec[] = { "None", "Ram size 2kb", "Ram size 8kb", "Ram size 32kb" };

char *rom_sz_vec[] = { "Rom size 32kb", "Rom size 64kb", "Rom size 128kb", 
						"Rom size 256kb", "Rom size 512kb", "Rom size 1mb", 
						"Rom size 2mb", "Rom size 4mb" };

/* Video-related globals */
Uint32 pal_grey[4]; // 32-bit palette for monochrome Game Boy
Uint32 pal_color[32*32*32]; // 32-bit palette for Color Game Boy
Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
Uint8 sgb_pal_map[20][18];
SDL_Surface *screen=NULL; // main window
SDL_Surface *back=NULL; // surface where pixels go

/* Debug-related globals */
int gbddb = 0; // enable/disable debug
int gbplay = 0; // control variable set when ROM loaded and 'play' command

/* Sound-related globals */
SDL_AudioSpec desired;
Sint16 *playbuf = NULL;
double freq_tbl[2048];
double freq_tbl_snd3[2048];
double freq_tbl_snd4[256];

/* Misc globals */
char *home_path;
char *file_path;
int use_boot_rom=0;
Uint32 gboy_mode=0; // Game Boy/Color Game Boy mode
Uint32 gboy_hw=0; // Game Boy/Color Game Boy hardware
FILE *boot_file;
FILE *rom_file;
Uint8 addr_sp[0xffff];
long ints_offs[] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
const char *types_vec[] = { "Rom only", "MBC1", "MBC1+RAM", "MBC1+RAM+BATTERY", NULL, "MBC2", "MBC2+BATTERY", NULL, "ROM+RAM", "ROM+RAM+BATTERY", NULL, "MMM01", "MMM01+RAM", "MMM01+RAM+BATTERY", NULL, "MBC3+TIMER+BATTERY", "MBC3+TIMER+RAM+BATTERY", "MBC3", "MBC3+RAM", "MBC3+RAM+BATTERY", NULL, NULL, NULL, NULL, NULL, "MBC5", "MBC5+RAM", "MBC5+RAM+BATTERY", "MBC5+RUMBLE", "MBC5+RUMBLE+RAM", "MBC5+RUMBLE+RAM+BATTERY" };
