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
#include "gboy_parse.h"

/*
 * Skips current line in 'str' starting at offset 'off'.
 * Returns offset to first character of next line, or
 * offset to end of string.
 */
int
skip_line(char *str, int off)
{
	while (str[off] != '\n') {
		off++;
		if (str[off] == '\0')
			return off;
	}
	while (str[off] == '\n')
		off++;

	return off;
}

/*
 * Skips blank characters in 'str' starting at offset 'off'.
 * Returns offset to first non-blank character.
 */
int
skip_blanks(char *str, int off)
{
	while (isspace(str[off]))
		off++;

	return off;
}

#ifndef VITA

void
gboy_exec_strs(int num_cmds, const char * const gboy_cmds[],
							 void (*gboy_funcs[])(int, char **))
{
	int m;

	/* Iterate through available commands and call function if coincidence */
	for (m=0; m < num_cmds; m++) {
		if (!strncmp(cmd_ptrs[0], gboy_cmds[m], 255)) {
			gboy_funcs[m](cmd_tail->num_args, cmd_ptrs);
			gboy_last_func=gboy_funcs[m];
			break;
		}
	}

	if (m == num_cmds)
		printf("Command not found...\n");
}

/*
 * Parse input and return number of arguments.
 * Identify arguments by empty spaces (ie. ' ' character).
 * Arrange an array of pointers to strings in cmd_ptrs.
 */
void
gboy_pars_strs(int num_cmds, const char * const gboy_cmds[],
			   void (*gboy_funcs[])(int, char **))
{
	int i = 0; // store number of arguments
	int m;
	char *ptr_buf = inp_buf; // pointer to beginning of string

	/* Ignore spaces and tabs until first character found */
	while (*ptr_buf == ' ' || *ptr_buf == '\t')
		ptr_buf++;

	ptr_dup = strndup(ptr_buf, 255);
	cmd_ptrs[i]=ptr_buf; // pointer for the first string; the command
	i=1; // first count for argc
	/* Convert each 'space' to 'end of string' (ie. ' ' -> '\0') and set pointers */
	while (*ptr_buf++ != '\0' && i <= MAX_STRS-1) {
		if (*ptr_buf==' ' || *ptr_buf=='\t') {
			*ptr_buf++='\0'; // replace with '\0' and advance to next character
			while (*ptr_buf==' ' || *ptr_buf=='\t')
				ptr_buf++;
			if (*ptr_buf == '\n' || *ptr_buf == '\0')
				break;
			else
				cmd_ptrs[i++]=ptr_buf;
		}
		else if (*ptr_buf == '\n')
			*ptr_buf++ = '\0';
	}
	*ptr_buf = '\0';

	/* Did we get the maximum strings? */
	if (i<MAX_STRS-1) /* no */
		*(ptr_buf-1)='\0', cmd_ptrs[i+1]=NULL; // for reference

	/* Queue commands */
	if (strncmp(ptr_dup, "\n", 255))
		cmd_add_que(ptr_dup, gboy_funcs[i], i, cmd_ptrs);

	gboy_exec_strs(num_cmds, gboy_cmds, gboy_funcs);
}

#endif
