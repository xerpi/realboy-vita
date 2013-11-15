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

FILE *
create_save(char *file_str)
{
	FILE *file_ptr;

	chdir("saves");

	/* Create file and open it */
	file_ptr = fopen(file_str, "w+");

	chdir("..");

	return file_ptr;
}

FILE *
open_save_try(char *file_str)
{
	FILE *file_ptr;

	chdir("saves");

	/* Try to open file */
	file_ptr = fopen(file_str, "r+");

	chdir("..");

	return file_ptr;
}

long
get_file_size(FILE *file)
{
	long file_size;

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	return file_size;
}

void
create_dir(char *dir_name, Uint32 mode_mask)
{
	if ( (mkdir(dir_name, (mode_t)mode_mask)) == -1)
		perror("Create Directory: ");
}

void
change_cur_dir(char *dir_path)
{
	if ( (chdir(dir_path)) == -1)
		perror("Change Directory: ");
}

char *
get_home_path()
{
	static char *ptr_hpath;

	/* XXX Can this fail? */
	if ( (ptr_hpath = getenv("HOME")) == NULL)
		perror("Home directory: ");

	return ptr_hpath;
}

int
search_file_dir(char *file, char *dir_path)
{
	DIR *home_dir = opendir(dir_path);
	struct dirent *dir_ent;
	while ( (dir_ent=readdir(home_dir)) != NULL) {
		if (!(strncmp(dir_ent->d_name, file, 9)))
			break;
	}
	closedir(home_dir);

	if (dir_ent != NULL)
		return 1;
	else
		return 0;
}
