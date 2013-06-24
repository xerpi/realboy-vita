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

char gddb_msg[] = "\nThis is gddb\n";
char *gddb_cmds[] = { "print", "show", "disasm", "step", "break", "help", "cont" };
/* Functions to handle commands */
extern void gddb_print(int, char **);
extern void gddb_show(int, char **);
extern void gddb_step(int, char **);
extern void gddb_cont(int, char **);
extern void gddb_dasm(int, char **);
extern void gddb_break(int, char **);
extern void gddb_help(int, char **);
extern void gddb_disasm(int, int);
extern char gddb_buf[512];
#ifdef USE_X86_64_ASM
extern long regs_sets;
#else
struct regs_sets {
	union regs {
		Uint8 UByte[2];
		Sint8 SByte[2];
		Uint16 UWord;
		Sint16 SWord;
	} regs[6];
};
extern struct regs_sets regs_sets;
#endif

char *gboy_pc;
char *op_rec;
long gddb_contil=-1;
int gddb_loop=1;
int gddb_tmp=0;

void (*gddb_cmds_funcs[7])(int, char **) = { gddb_print, gddb_show, gddb_dasm, gddb_step, gddb_break, gddb_help, gddb_cont };

void
gddb_reset()
{
	gbddb=0;
	gddb_contil=-1;
	gddb_loop=1;
	gddb_tmp=0;
}

void
gddb_main(int null_null, char *ptr_gboy_pc, char *ptr_op_rec)
{
	int i;
	char *cmd_ptrs[MAX_STRS+2]; // array of pointers to strings (last two are NULL)

	if (gddb_contil != -1) {
#ifdef USE_X86_64_ASM
		if (*(&regs_sets+5) == gddb_contil)
#else
		if (regs_sets.regs[10].UWord == (Uint16)gddb_contil)
#endif
		{
			if (gddb_tmp) {
				gddb_tmp--; goto out;}
			printf("Breakpoint at 0x%x\n", (int)gddb_contil), gddb_contil = -1;
		}
		else
			goto out;
	}
	gboy_pc = ptr_gboy_pc;
	op_rec = ptr_op_rec;
	gddb_disasm(1, 0);

	/* Receive commands */
 	/* Output message and input string parse string and get number */
	while (gddb_loop)
		gboy_interp("gddb> ", 7, gddb_cmds, gddb_cmds_funcs, gddb_step);
out:
	gddb_loop = 1;

}
