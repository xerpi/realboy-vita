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
#include "main.h"

PSP2_MODULE_INFO(0, 0, "realboy");

/* libc functions */

static char current_dir[1024];

size_t strnlen(register const char *s, size_t maxlen)
{
	register const char *e;
	size_t n;

	for (e = s, n = 0; *e && n < maxlen; e++, n++)
		;
	return n;
}

char *strndup(const char *s, size_t n)
{
	char *result;
	size_t len = strlen (s);

	if (n < len)
		len = n;

	result = (char *) malloc (len + 1);
	if (!result)
		return 0;

	result[len] = '\0';
	return (char *) memcpy (result, s, len);
}

char *basename(const char *name)
{
	const char *base = name;

	while (*name) {
		if (*name++ == '/') {
			base = name;
		}
	}
	return (char *) base;
}

int chdir(const char *path)
{
	strcpy(current_dir, path);
	return 0;
}

int mkdir(const char *pathname, mode_t mode)
{
	return sceIoMkdir(pathname, mode);
}

char *getenv(const char *name)
{
	if (strcmp(name, "HOME") == 0) {
		return "cache0:/VitaDefilerClient/Documents";
	}
	return NULL;
}

/*
 * Main function.
 */
int main()
{
	printf("\nRealBoy %s\n", "0.2.2");

	vid_scale(3);
	vid_toggle_fullscreen();
	use_boot_rom = 0;

	if ( (rom_file=fopen("cache0:/VitaDefilerClient/Documents/rom.gb", "r")) == NULL)
		printf("\nError: rom_file: %p\n", rom_file);
	else
		file_path = strndup("cache0:/VitaDefilerClient/Documents", 256);


	if (rom_file != NULL)	{
		//init_conf();
		int ret_val; // value returned from emulation
		/* Start Virtual Machine */
		ret_val = start_vm();
		/* Error returned if file not valid */
		if (ret_val == -1)
			printf("File not a gb binary\n\n");
		else
			printf("\nThank you for using RealBoy!\n\n");
	}

	sceKernelExitProcess(0);
	return 0;
}
