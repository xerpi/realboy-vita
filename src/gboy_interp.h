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
#define MAX_CHAR 512

/* Pointers to beginning, end and current character of input buffer */
struct cmd_line {
	char *ptr_cur; // pointer to current byte in buffer
	char *ptr_end; // pointer at the end of the buffer
	char *ptr_beg; // pointer at the beginning of buffer
} cmd_line;

static struct cmd_stack cmd_head; // head of the list
static struct cmd_stack *cmd_cur = NULL; // pointer to current command
struct cmd_stack *cmd_tail = &cmd_head; // pointer to last command
extern int gboy_pars_strs(int num_cmds, const char * const gboy_cmds[], void (*gboy_funcs[])(int, char **));
