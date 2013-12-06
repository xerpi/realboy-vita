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
#include "gboy_keys.h"

static void
print_char(int charac)
{
	if (isprint(charac) && charac != ' ')
		printf("'%c'\n", charac);

	switch (charac) {
		case '\b':
			printf("'Backspace'\n");
			break;
		case '\t':
			printf("'Tab'\n");
			break;
		case '\n':
			printf("'Enter'\n");
			break;
		case ' ':
			printf("'Space'\n");
			break;
	}
}

static int
valid_key(int key_ascii, int key)
{
	if ((isprint(key_ascii) || key_ascii == '\t' || key_ascii == '\n' || key_ascii == '\b') && (!isdigit(key_ascii)))
	{
		if (key != A_KEY)
			if (key_ascii == key_binds_ascii[A_KEY])
				return 1;
		if (key != B_KEY)
			if (key_ascii == key_binds_ascii[B_KEY])
				return 1;
		if (key != START_KEY)
			if (key_ascii == key_binds_ascii[START_KEY])
				return 1;
		if (key != SELECT_KEY)
			if (key_ascii == key_binds_ascii[SELECT_KEY])
				return 1;
		return 0;
	}
	else
		return 2;
}

static void
joy_rebind_SDL(int key, int ascii)
{
	switch (ascii) {
		case 'q':
			key_binds_SDL[key] = SDLK_q;
			break;
		case 'w':
			key_binds_SDL[key] = SDLK_w;
			break;
		case 'e':
			key_binds_SDL[key] = SDLK_e;
			break;
		case 'r':
			key_binds_SDL[key] = SDLK_r;
			break;
		case 't':
			key_binds_SDL[key] = SDLK_t;
			break;
		case 'y':
			key_binds_SDL[key] = SDLK_y;
			break;
		case 'u':
			key_binds_SDL[key] = SDLK_u;
			break;
		case 'i':
			key_binds_SDL[key] = SDLK_i;
			break;
		case 'o':
			key_binds_SDL[key] = SDLK_o;
			break;
		case 'p':
			key_binds_SDL[key] = SDLK_p;
			break;
		case 'a':
			key_binds_SDL[key] = SDLK_a;
			break;
		case 's':
			key_binds_SDL[key] = SDLK_s;
			break;
		case 'd':
			key_binds_SDL[key] = SDLK_d;
			break;
		case 'f':
			key_binds_SDL[key] = SDLK_f;
			break;
		case 'g':
			key_binds_SDL[key] = SDLK_g;
			break;
		case 'h':
			key_binds_SDL[key] = SDLK_h;
			break;
		case 'j':
			key_binds_SDL[key] = SDLK_j;
			break;
		case 'k':
			key_binds_SDL[key] = SDLK_k;
			break;
		case 'l':
			key_binds_SDL[key] = SDLK_l;
			break;
		case 'z':
			key_binds_SDL[key] = SDLK_z;
			break;
		case 'x':
			key_binds_SDL[key] = SDLK_x;
			break;
		case 'c':
			key_binds_SDL[key] = SDLK_c;
			break;
		case 'v':
			key_binds_SDL[key] = SDLK_v;
			break;
		case 'b':
			key_binds_SDL[key] = SDLK_b;
			break;
		case 'n':
			key_binds_SDL[key] = SDLK_n;
			break;
		case 'm':
			key_binds_SDL[key] = SDLK_m;
			break;
		case '\b':
			key_binds_SDL[key] = SDLK_BACKSPACE;
			break;
		case '\t':
			key_binds_SDL[key] = SDLK_TAB;
			break;
		case '\n':
			key_binds_SDL[key] = SDLK_RETURN;
			break;
		case ' ':
			key_binds_SDL[key] = SDLK_SPACE;
			break;
	}
}

static void
joy_rebind_keys(int keys)
{
	int new_ascii;

	if (keys == ALL_KEYS) {
		joy_rebind_keys(A_KEY);
		joy_rebind_keys(B_KEY);
		joy_rebind_keys(START_KEY);
		joy_rebind_keys(SELECT_KEY);
	}
	else {
		int inval_key;
		do {
			printf("Choose new key binding for %s\n", key_binds_strs[keys]);
			printf("-> ");
			new_ascii = getch();
			inval_key = valid_key(new_ascii, keys);
			if (inval_key)
				printf("%s\n", key_binds_errs[inval_key-1]);
		} while (inval_key != 0);
		printf("New binding: %s -> ", key_binds_strs[keys]);
		print_char(new_ascii);
		printf("\n");
		key_binds_ascii[keys] = (char)new_ascii;
		joy_rebind_SDL(keys, new_ascii);
	}
}

int
joy_remap(char key_ascii, int key_remap)
{
	int inval_key;

	inval_key = valid_key((int)key_ascii, key_remap);
	if (inval_key) {
		printf("%s\n", key_binds_errs[inval_key-1]);
		return 0;
	}

	key_binds_ascii[key_remap] = key_ascii;
	joy_rebind_SDL(key_remap, key_ascii);

	return 1;
}

void
joy_chbinds()
{
	int key_ascii='a';

	while (1)
	{
		printf("\n\n===============================================================\n");
		printf("Current key bindings are:\n");
		printf("A: ");
		print_char(key_binds_ascii[0]);
		printf("B: ");
		print_char(key_binds_ascii[1]);
		printf("START: ");
		print_char(key_binds_ascii[2]);
		printf("SELECT: ");
		print_char(key_binds_ascii[3]);
		printf("\n\nPress current key binding you would like to configure, or...\n");
		printf("Press '8' if you would like to rebind all keys, or...\n");
		printf("Press '1' to return to emulation.\n");
		printf("\n===============================================================\n");
		printf("-> ");
		key_ascii = getch();
		if (key_ascii == '8') {
			joy_rebind_keys(ALL_KEYS);
			break;
		}
		else if (key_ascii == key_binds_ascii[0]) {
			joy_rebind_keys(A_KEY);
		}
		else if (key_ascii == key_binds_ascii[1]) {
			joy_rebind_keys(B_KEY);
		}
		else if (key_ascii == key_binds_ascii[2]) {
			joy_rebind_keys(START_KEY);
		}
		else if (key_ascii == key_binds_ascii[3]) {
			joy_rebind_keys(SELECT_KEY);
		}
		else if (key_ascii == '1')
			break;
		else
			printf("Invalid key\n");
	}

	printf("\n===============================================================\n");
	printf("New key bindings are:\n");
	printf("A: ");
	print_char(key_binds_ascii[0]);
	printf("B: ");
	print_char(key_binds_ascii[1]);
	printf("START: ");
	print_char(key_binds_ascii[2]);
	printf("SELECT: ");
	print_char(key_binds_ascii[3]);
	printf("\nCONTINUING EMULATION...\n");
}
