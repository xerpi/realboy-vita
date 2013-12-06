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
#include "gboy_interp.h"

int
getch()
{
	int ch;
	struct termios oldt;
	struct termios newt;

	tcgetattr(STDIN_FILENO, &oldt); // store old settings
	newt = oldt; // copy old settings to new settings
	newt.c_lflag &= ~(ICANON | ECHO); // make one change to old settings in new settings
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); // apply the new settings immediatly
	ch = getchar(); // standard getchar call
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // reapply the old settings
	
	return ch; // return received char
}

static void
cmd_auto_compl()
{
	/* Current working directory string */
	char *cwd;

	cwd=getcwd(NULL, 0);
	while ((*cmd_line.ptr_cur != ' '))
		cmd_line.ptr_cur--;

	if ((chdir(cmd_line.ptr_cur))==-1)
		return;

	


	free(cwd);
}

static void
cmd_prev_que()
{
	int i, m;
	char *ptr_buf;

	if (cmd_cur == NULL)
		;
	else {
		if (cmd_cur->ptr_fw != NULL) {
			cmd_cur = cmd_cur->ptr_fw;
			i=(strnlen(cmd_line.ptr_beg, 255)-strnlen(cmd_line.ptr_cur, 255));
			for (ptr_buf = cmd_line.ptr_cur; ptr_buf < cmd_line.ptr_end; ptr_buf++)
				putchar(' '), i++;
			while (i--)
				putchar(8), putchar(' '), putchar(8);
			/* Empty input buffer */
			for (ptr_buf = &inp_buf[511]; ptr_buf >= inp_buf; ptr_buf--)
				*ptr_buf=0;
			ptr_buf = cmd_cur->cmd_buf;
			m = strnlen(cmd_cur->cmd_buf, 255);
			for (i = 0; i < m; i++)
				putchar(*ptr_buf), inp_buf[i] = *ptr_buf++;
			cmd_line.ptr_beg = inp_buf;
			cmd_line.ptr_end = cmd_line.ptr_cur = inp_buf + i;
		}
	}
}

static void
cmd_next_que()
{
	int i, m;
	char *ptr_buf;

	if (cmd_cur == NULL) {
		cmd_cur = cmd_tail;
		if (cmd_cur->cmd_buf != NULL) {
			i=(strnlen(cmd_line.ptr_beg, 255)-strnlen(cmd_line.ptr_cur, 255));
			for (ptr_buf = cmd_line.ptr_cur; ptr_buf < cmd_line.ptr_end; ptr_buf++)
				putchar(' '), i++;
			while (i--)
				putchar(8), putchar(' '), putchar(8);
			/* Empty input buffer */
			for (ptr_buf = &inp_buf[511]; ptr_buf >= inp_buf; ptr_buf--)
				*ptr_buf=0;
			ptr_buf = cmd_cur->cmd_buf;
			m = strnlen(cmd_cur->cmd_buf, 255);
			for (i = 0; i < m; i++)
				putchar(*ptr_buf), inp_buf[i] = *ptr_buf++;
			cmd_line.ptr_beg = inp_buf;
			cmd_line.ptr_end = cmd_line.ptr_cur = inp_buf + i;
		}
	}
	else {
		if (cmd_cur->ptr_bk != NULL) {
			cmd_cur = cmd_cur->ptr_bk;
			if (cmd_cur->cmd_buf == NULL) 
				cmd_cur = cmd_cur->ptr_fw;
			else {
				i=(strnlen(cmd_line.ptr_beg, 255)-strnlen(cmd_line.ptr_cur,255));
				for (ptr_buf = cmd_line.ptr_cur; ptr_buf < cmd_line.ptr_end; ptr_buf++)
					putchar(' '), i++;
				while (i--)
					putchar(8), putchar(' '), putchar(8);
				/* Empty input buffer */
				for (ptr_buf = &inp_buf[511]; ptr_buf >= inp_buf; ptr_buf--)
					*ptr_buf=0;
				ptr_buf = cmd_cur->cmd_buf;
				m = strnlen(cmd_cur->cmd_buf, 255);
				for (i = 0; i < m; i++)
					putchar(*ptr_buf), inp_buf[i] = *ptr_buf++;
				cmd_line.ptr_beg = inp_buf;
				cmd_line.ptr_end = cmd_line.ptr_cur = inp_buf + i;
			}
		}
	}
}

void
cmd_add_que(char *ptr_buf, void *cmd_func_addr, int cmd_nargs, char **cmd_args)
{
	struct cmd_stack *ptr_stack = &cmd_head;

	while (ptr_stack->ptr_fw != NULL)
		ptr_stack = ptr_stack->ptr_fw;

	ptr_stack->ptr_fw = malloc(sizeof(struct cmd_stack));
	ptr_stack->ptr_fw->ptr_bk = ptr_stack;
	ptr_stack = ptr_stack->ptr_fw;
	ptr_stack->cmd_fun = cmd_func_addr;
	ptr_stack->num_args = cmd_nargs;
	ptr_stack->cmd_ptrs = cmd_args;
	ptr_stack->ptr_fw = NULL;
	ptr_stack->cmd_buf = ptr_buf;
	cmd_tail = ptr_stack;
}

/*
 * Controls the input by the user and calls gboy_pars_strs to parse it.
 */
void
gboy_interp(char *ptr_str2, int num_cmds, const char * const gboy_cmds[], void (*gboy_funcs[])(int, char **), void (*gboy_def)(int, char **))
{
	char *ptr_buf; // pointer to input buffer
	int get_char; // get each character from input
	int i; // control variable

	/* Initialize input buffer */
	for (cmd_line.ptr_cur = &inp_buf[511]; cmd_line.ptr_cur >= inp_buf; cmd_line.ptr_cur--)
		*cmd_line.ptr_cur = 0;

	/* Initialize */
	cmd_line.ptr_end = cmd_line.ptr_beg = ++cmd_line.ptr_cur;
	cmd_cur = NULL;

	write(1, ptr_str2, 7); // command prompt XXX use strnlen()
	
	/* 
	 * Get a stream of characters until EOL.
	 */
	while ( (get_char = getch()) != '\n') {
		if (cmd_line.ptr_end - cmd_line.ptr_beg >= MAX_CHAR) {
			printf("Buffer overrun!\n"); return;
		}
		/* If printable character */
		if (isprint(get_char)) {
			/* If current character not empty make place for new character */
			if (*cmd_line.ptr_cur != 0) {
				cmd_line.ptr_end++; // advance pointer to end of string
				ptr_buf = cmd_line.ptr_end; // point to end of string
				/* Displace characters one to the right; make space for new char */
				while (ptr_buf > cmd_line.ptr_cur)
					*ptr_buf-- = *(ptr_buf-1);
				/* Write new character; advance current-character pointer */
				*ptr_buf = (char)get_char, cmd_line.ptr_cur++, i=0;
				/* Print new string */
				while (ptr_buf < cmd_line.ptr_end)
					putchar((int)(*ptr_buf++)), i++;
				/* Place cursor */
				while (--i)
					putchar(8);
			}
			/* Put new character and advance pointers */
			else {
				putchar((int)(*cmd_line.ptr_cur = get_char));
				cmd_line.ptr_cur++, cmd_line.ptr_end++;
			}
		}
		/* Not printable, so treat special cases */
		else {
			switch (get_char) {
				case '\t':
					//cmd_auto_compl();
					break;
				case 27:
					getch(); 
					switch (get_char = getch()) {
						case 65: // up
							cmd_next_que(); // next command on queue
							break;
						case 66: // down
							cmd_prev_que(); // previous command on queue
							break;
						case 67: // right
							if (cmd_line.ptr_cur < cmd_line.ptr_end) {
								putchar(*cmd_line.ptr_cur);
								cmd_line.ptr_cur++;
							}
							break;
						case 68: // left
							if (cmd_line.ptr_cur > cmd_line.ptr_beg) {
								putchar(8);
								cmd_line.ptr_cur--;
							}
							break;
						case 70: // start of string
							while (cmd_line.ptr_cur < cmd_line.ptr_end) {
								putchar(*cmd_line.ptr_cur);
								cmd_line.ptr_cur++;
							}
							break;
						case 72: // start of string
							while (cmd_line.ptr_cur > cmd_line.ptr_beg) {
								putchar(8);
								cmd_line.ptr_cur--;
							}
							break;
					}
					break;
				case 127:
				case 8:
					if (cmd_line.ptr_cur > cmd_line.ptr_beg && *cmd_line.ptr_cur != 0) {
						putchar(8);
						cmd_line.ptr_cur--;
						ptr_buf = cmd_line.ptr_cur;
						while (ptr_buf < cmd_line.ptr_end)
							*ptr_buf++ = *(ptr_buf+1), putchar(*ptr_buf);
						*ptr_buf = 0, putchar(' ');
						ptr_buf = --cmd_line.ptr_end;
						while (ptr_buf-- >= cmd_line.ptr_cur)
							putchar(8);
					}
					else if (cmd_line.ptr_cur > cmd_line.ptr_beg) {
						putchar(8);
						cmd_line.ptr_cur--;
						cmd_line.ptr_end--;
						putchar(' ');
						putchar(8);
						*cmd_line.ptr_cur = 0;
					}
					break;
			}
		}
	}

	while (*(cmd_line.ptr_end-1)==' ')
	{
		*--cmd_line.ptr_end=0;;
		putchar(8);
	}
	if (cmd_line.ptr_end<cmd_line.ptr_cur)
		cmd_line.ptr_cur=cmd_line.ptr_end;

	/* Put EOL */
	putchar(get_char);

	if (strncmp(inp_buf, "", 255))
		gboy_pars_strs(num_cmds, gboy_cmds, gboy_funcs);
	/* If NULL string don't bother calling parser; call the default command instead */
	else {
		if (gboy_def!=NULL)
			gboy_def(1, NULL);
	}

}
