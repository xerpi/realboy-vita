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

void
gboy_ddb(int num_args, char **ptr_ptrs)
{
	if (num_args>1)
		printf("Usage: debug\n");
	else {
		if (gbddb == 0)
			gbddb = 1;
		else
			gbddb = 0;
	}

	printf("gboy debug is %d\n", gbddb);
}

void
gboy_play(int num_args, char **ptr_ptrs)
{
	int ret_val;

	if (num_args > 1)
		printf("Usage: play\n");
	else {
		if (rom_fd > 0)
			gbplay = 1;
		else
			printf("First load a cartridge.\n");
	}
}

void
gboy_help(int num_args, char **ptr_ptrs)
{
	if (num_args > 1)
		printf("Usage: help\n");
	else
		printf("Commands: help, load, play, debug, fps, mode\n");
}

void
gboy_load(int num_args, char **ptr_ptrs)
{
	static char *rom_str;

	if (num_args!=2)
		printf("Usage: load 'file_path'\n");
	else {
		if (rom_fd!=-1 && rom_fd!=0) {
			close(rom_fd);
			printf("File %s closed\n", rom_str);
		}
		if ( (rom_fd=open(ptr_ptrs[1], O_RDONLY)) == -1)
			perror(ptr_ptrs[1]);
		else
			printf("\nFile '%s' opened. (Use 'play' command to start emulation)\n", ptr_ptrs[1]), 
			rom_str=strndup(ptr_ptrs[1], 255);
	}
}

void
gboy_selmode(int num_args, char **ptr_ptrs)
{
	if (num_args!=2)
		printf("Available modes are: \"DMG\" and \"CGB\"\n");
	else {
		if (!(strncmp(ptr_ptrs[1], "CGB", 255)) || !(strncmp(ptr_ptrs[1], "cgb", 255)))
			printf("Mode set to CGB\n"), gboy_mode=1;
		else if (!(strncmp(ptr_ptrs[1], "DMG", 255)) || !(strncmp(ptr_ptrs[1], "dmg", 255)))
			printf("Mode set to DMG\n"), gboy_mode=0;
		else
			printf("Available modes are: \"DMG\" and \"CGB\"\n");
	}
}

extern int frames_per_second;
void
gboy_fps(int num_args, char **ptr_ptrs)
{
	int fps;

	if (num_args != 2)
		printf("Usage: fps '0<int<=60'\n");
	else {
		fps=atoi(ptr_ptrs[1]);
		if (fps>0 && fps<=60)
			printf("FPS set to %d\n", frames_per_second=fps);
		else
			printf("Bad integer %d for FPS\n", fps);
	}
}
