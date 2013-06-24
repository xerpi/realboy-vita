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
#define STR_OFF 8 // string offset

char gddb_buf[512];
extern int gddb_tmp;
extern long gddb_contil;
extern long gddb_loop;
#ifdef USE_X86_64_ASM
extern long regs_sets;
extern char z80_cb;
extern char z80_ldex;
#else
struct z80_set {
	Uint8 format[8];
	char name[16];
	long length;
	void (*func)(struct z80_set *);
};
struct regs_sets {
	union regs {
		Uint8 UByte[2];
		Sint8 SByte[2];
		Uint16 UWord;
		Sint16 SWord;
	} regs[10];
};
extern struct regs_sets regs_sets;
extern struct z80_set z80_ldex[512];
#endif
//extern char *addr_sp;
extern long *addr_sp_bases[];
extern long addr_sp_ptrs[16];
extern char *gboy_pc;
extern char *op_rec;
extern void memdump32(int *mem_arg, char *buf_arg, char num_words, int num_bytes, 
											int num_base);

char *gddb_regs[] = { "AF = ", "BC = ", "DE = ", "HL = ", "SP = ", "PC = ", NULL };

struct gddb_registers {
	char *gddb_str;
	int gddb_next;
};

struct gddb_registers gddb_ioregs[] = 
{ 
	"DIV = ", 1,
	"TIMA = ", 1,
	"TMA = ", 1,
	"TAC = ", 8,
	"IF = ", 240,
	"IE = ", 0,
	NULL, 0
};

struct gddb_registers gddb_lcdregs[] = 
{ 
	"LCDC = ", 1,
	"STAT = ", 1,
	"SCY = ", 1,
	"SCX = ", 1,
	"LY = ", 1,
	"LYC = ", 1,
	"OAM DMA = ", 1,
	"BGP = ", 1,
	"OBP0 = ", 1,
	"OBP1 = ", 1,
	"WY = ", 1,
	"WX = ", 1,
	NULL, 0
};

struct gddb_registers gddb_sndregs[] = 
{ 
	"NR10 = ", 1,
	"NR11 = ", 1,
	"NR12 = ", 1,
	"NR13 = ", 1,
	"NR14 = ", 2,
	"NR21 = ", 1,
	"NR22 = ", 1,
	"NR23 = ", 1,
	"NR24 = ", 1,
	"NR30 = ", 1,
	"NR31 = ", 1,
	"NR32 = ", 1,
	"NR33 = ", 1,
	"NR34 = ", 2,
	"NR41 = ", 1,
	"NR42 = ", 1,
	"NR43 = ", 1,
	"NR44 = ", 1,
	"NR50 = ", 1,
	"NR51 = ", 1,
	"NR52 = ", 1,
	NULL, 0
};

void
gddb_cont(int num_args, char **ptr_ptrs)
{
	gbddb=0;
	gddb_contil=-1;
	gddb_loop=0;
	gddb_tmp=0;

	printf("\n\nContinuing execution...\nPress 'g' to drop to debugger.\n\n");
}

void
gddb_disasm(int num_ops, int rep)
{
	unsigned char *gboy_pc_local = gboy_pc;
	unsigned char *op_rec_local = op_rec;
	int i;

#ifdef USE_X86_64_ASM
	long pc_temp = *((&regs_sets)+5);
#else
	long pc_temp = regs_sets.regs[PC].UWord;
#endif


	while (num_ops--) {
		if ((unsigned char)(*gboy_pc_local) == 0xcb) {
			//printf("Escape to: \n");
			gboy_pc_local++;
			#ifdef USE_X86_64_ASM
				op_rec_local = (char *)(&z80_cb)+((int)((unsigned char)*gboy_pc_local)<<5);
			#else
				op_rec_local = (char *)((z80_ldex+256)+(*gboy_pc_local));
			#endif
			pc_temp++;
			memdump32((int *)&pc_temp, gddb_buf, 1, 2, 16);
			printf("%s\t", gddb_buf);
			printf("%s", op_rec_local+STR_OFF);
		}
		else {
			memdump32((int *)&pc_temp, gddb_buf, 1, 2, 16);
			printf("%s\t", gddb_buf);
			if (*op_rec_local == 0xea)
			{
				printf("%s", op_rec_local+STR_OFF);
				memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
				printf(" (%s), ", gddb_buf);
				printf("A");
			}
			else if (*op_rec_local == 0x08)
			{
				printf("%s", op_rec_local+STR_OFF);
				printf(" SP, ");
				memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
				printf("*%s", gddb_buf);
			}
			else if (!strncmp(op_rec_local+STR_OFF,"ld",255)||!strncmp(op_rec_local+STR_OFF, "ldi", 255)||!strncmp(op_rec_local+STR_OFF, "ldd", 255)) 
			{
				printf("%s", op_rec_local+STR_OFF);
				switch (*((int *)(op_rec_local+24))) {
					case WORD_W:
						if (*(op_rec_local+1) == REG) {
							if (*(op_rec_local+2) == AF)
								printf(" AF, ");
							else if (*(op_rec_local+2) == BC)
								printf(" BC, ");
							else if (*(op_rec_local+2) == DE)
								printf(" DE, ");
							else if (*(op_rec_local+2) == HL)
								printf(" HL, ");
							else if (*(op_rec_local+2) == SP)
								printf(" SP, ");
							else
								printf(" PC, ");
						}
						else if (*(op_rec_local+1) == IMM_IND) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
								printf("(%s)", gddb_buf);
						}
						if (*(op_rec_local+3) == REG) {
							if (*(op_rec_local+4) == AF)
								printf("AF");
							else if (*(op_rec_local+4) == BC)
								printf("BC");
							else if (*(op_rec_local+4) == DE)
								printf("DE");
							else if (*(op_rec_local+4) == HL)
								printf("HL");
							else if (*(op_rec_local+4) == SP)
								printf("SP");
							else
								printf("PC");
						}
						else if (*(op_rec_local+3) == IMM_IND) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
								printf("(%s)", gddb_buf);
								break;
						}
						else if (*(op_rec_local+3) == IMM) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
								printf("%s", gddb_buf);
								break;
						}
					break;
					case BYTE_B:
						if (*(op_rec_local+1) == REG) {
							if (*(op_rec_local+2) == A)
								printf(" A, ");
							else if (*(op_rec_local+2) == B)
								printf(" B, ");
							else if (*(op_rec_local+2) == D)
								printf(" D, ");
							else if (*(op_rec_local+2) == H)
								printf(" H, ");
							else if (*(op_rec_local+2) == C)
								printf(" C, ");
							else if (*(op_rec_local+2) == E)
								printf(" E, ");
							else if (*(op_rec_local+2) == L)
								printf(" L, ");
							else
								printf(" F, ");
						}
						else if (*(op_rec_local+1) == IMM_IND) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
								printf("*%s", gddb_buf);
						}
						else if (*(op_rec_local+1) == 0x18) {
							#ifdef USE_X86_64_ASM
								memdump32((int *)((char *)(&regs_sets)+C), gddb_buf, 1, 1, 16);
							#else
								memdump32((int *)((char *)(&regs_sets.regs[BC].UByte[C])),gddb_buf,1,1,16);
							#endif
								printf(" (0xff%s), ", gddb_buf+2);
						}
						else if (*(op_rec_local+1) == 0x1) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
								printf(" (0xff%s), ", gddb_buf+2);
						}
						if (*(op_rec_local+3) == REG) {
							if (*(op_rec_local+4) == A)
								printf("A");
							else if (*(op_rec_local+4) == B)
								printf("B");
							else if (*(op_rec_local+4) == D)
								printf("D");
							else if (*(op_rec_local+4) == H)
								printf("H");
							else if (*(op_rec_local+4) == C)
								printf("C");
							else if (*(op_rec_local+4) == E)
								printf("E");
							else if (*(op_rec_local+4) == L)
								printf("L");
							else
								printf("F");
						}
						else if (*(op_rec_local+3) == IMM_IND) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
								printf("*%s", gddb_buf);
								break;
						}
						else if (*(op_rec_local+3) == IMM) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
								printf("%s", gddb_buf);
								break;
						}
						else if (*(op_rec_local+3) == 0x18) {
							#ifdef USE_X86_64_ASM
								memdump32((int *)((char *)(&regs_sets)+C), gddb_buf, 1, 1, 16);
							#else
								memdump32((int *)((char *)(&regs_sets.regs[DE].UByte[E])),gddb_buf,1,1,16);
							#endif
								printf("(0xff%s)", gddb_buf+2);
								break;
						}
						else if (*(op_rec_local+3) == 0x1) {
								memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
								printf("(0xff%s)", gddb_buf+2);
								break;
						}
					}
				}
			else if (*op_rec_local == 0x18 || *op_rec_local == 0x20 || *op_rec_local == 0x28 || 
							 *op_rec_local == 0x30 || *op_rec_local == 0x38) 
			{
				printf("%s ", op_rec_local+STR_OFF);
				printf("0x%hx", (int)((pc_temp+2)+((signed char)gboy_pc_local[1])));
			}
			else if ((unsigned char)*op_rec_local == 0xc3 || (unsigned char)*op_rec_local == 0xca || 
					 (unsigned char)*op_rec_local == 0xc2 || (unsigned char)*op_rec_local == 0xd2 || 
					 (unsigned char)*op_rec_local == 0xda || (unsigned char)*op_rec_local == 0xdc || 
					 (unsigned char)*op_rec_local == 0xcd || (unsigned char)*op_rec_local == 0xcc || 
					 (unsigned char)*op_rec_local == 0xc4 || (unsigned char)*op_rec_local == 0xd4)
			{
				printf("%s ", op_rec_local+STR_OFF);
				memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
				printf("%s", gddb_buf);
			}
			else if ((unsigned char)*op_rec_local == 0xe6 || (unsigned char)*op_rec_local == 0xee || 
					 (unsigned char)*op_rec_local == 0xf6 || (unsigned char)*op_rec_local == 0xfe)
			{
				printf("%s ", op_rec_local+STR_OFF);
				memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 1, 16);
				printf("%s", gddb_buf);
			}
			else if ((unsigned char)*op_rec_local == 0xe8)
			{
				printf("%s ", op_rec_local+STR_OFF);
				memdump32((int *)(gboy_pc_local+1), gddb_buf, 1, 2, 16);
				printf("%s", gddb_buf);
			}
			else
				printf("%s", op_rec_local+STR_OFF);
		}
		printf("\t\t( ");
		for (i=0; i<*(op_rec_local+6); i++) {
			memdump32((int *)(gboy_pc_local+i), gddb_buf, 1, 1, 16);
			printf("%s ", gddb_buf);
		}
		printf(")");
		gboy_pc_local += *(op_rec_local+6);
		pc_temp += *(op_rec_local+6);
		op_rec_local = (char *)(&z80_ldex)+((int)((unsigned char)*gboy_pc_local)<<5);

		printf("\n");

		if (rep == 1)
		if (num_ops == 0) {
			puts("Any key to continue; 'q' to stop...");
			rep=getch();//("%c", (char *)&rep);
			if ((char)rep != 'q')
				rep = 1, num_ops = 10;
		}
	}
}

void
gddb_break(int num_args, char **ptr_ptrs)
{
	long ptr_num=0;
	int str_len;

	if (num_args==2) {
		if (addr_is_hex(ptr_ptrs[1], strnlen(ptr_ptrs[1], 255)))
			str_hex_to_num(ptr_ptrs[1]+2, &ptr_num, 
						  (str_len = strnlen(ptr_ptrs[1]+2, 255)) > 4 ? 0 : str_len);
		else
			ptr_num = atoi(ptr_ptrs[1]); 
		if (ptr_num==0)
			printf("Bad address \"%s\"\n", ptr_ptrs[1]), printf("No breakpoint set\n");
		else
			gddb_contil = ptr_num, printf("Breakpoint set at: 0x%x\n", (unsigned int)gddb_contil);
	}	
	else
		printf("Usage: break 'address'\n"), gddb_contil = -1, printf("Breakpoint unset\n");
}

void
gddb_dasm(int num_args, char **ptr_ptrs)
{
	if (num_args == 1)
		gddb_disasm(10, 1);
}

void
gddb_print(int num_args, char **ptr_ptrs)
{
	long ptr_num=0;
	int str_len;

	if (num_args==2) {
		if (addr_is_hex(ptr_ptrs[1], strnlen(ptr_ptrs[1], 255)))
			str_hex_to_num(ptr_ptrs[1]+2, &ptr_num, 
						  (str_len = strnlen(ptr_ptrs[1]+2, 255)) > 4 ? 0 : str_len);
		else
			ptr_num = atoi(ptr_ptrs[1]); 
		if (ptr_num==0)
			printf("Bad address \"%s\"\n", ptr_ptrs[1]);
		else {
			printf("(0x%x) = ", (unsigned int)ptr_num);
			ptr_num = (long)(addr_sp_ptrs[(ptr_num>>12)&0x3c]) + ptr_num;
			memdump32((int *)(ptr_num), gddb_buf, 1, 1, 16);
			printf("%s\n", gddb_buf);
		}
	}	
	else
		printf("Usage: print 'address'\n");

}

void
gddb_show(int num_args, char **ptr_ptrs)
{
	int i=0;
	char *ptr_addr;

	if (num_args != 2)
		printf("Usage: show [regs, ioregs, lcdregs, sndregs]\n");
	else {
		if (!strncmp("regs", ptr_ptrs[1], 255)) {
			while (gddb_regs[i] != NULL) {
				#ifdef USE_X86_64_ASM
					memdump32((int *)((&regs_sets)+i), gddb_buf, 1, 2, 16);
				#else
					memdump32((int *)((&regs_sets.regs[i*2].UWord)),gddb_buf,1,2,16);
				#endif
				printf("%s%s\n", gddb_regs[i++], gddb_buf);
			}
		}
		else if (!strncmp("ioregs", ptr_ptrs[1], 255)) {
			i=0;
			ptr_addr = addr_sp+0xff04;
			while (gddb_ioregs[i].gddb_str != NULL) {
				memdump32((int *)ptr_addr, gddb_buf, 1, 1, 16);
				printf("%s%s\n", gddb_ioregs[i].gddb_str, gddb_buf);
				ptr_addr += gddb_ioregs[i++].gddb_next;
			}
		}
		else if (!strncmp("lcdregs", ptr_ptrs[1], 255)) {
			i=0;
			ptr_addr = addr_sp+0xff40;
			while (gddb_lcdregs[i].gddb_str != NULL) {
				memdump32((int *)ptr_addr, gddb_buf, 1, 1, 16);
				printf("%s%s\n", gddb_lcdregs[i].gddb_str, gddb_buf);
				ptr_addr += gddb_lcdregs[i++].gddb_next;
			}
		}
		else if (!strncmp("sndregs", ptr_ptrs[1], 255)) {
			i=0;
			ptr_addr = addr_sp+0xff10;
			while (gddb_sndregs[i].gddb_str != NULL) {
				memdump32((int *)ptr_addr, gddb_buf, 1, 1, 16);
				printf("%s%s\n", gddb_sndregs[i].gddb_str, gddb_buf);
				ptr_addr += gddb_sndregs[i++].gddb_next;
			}
			ptr_addr = addr_sp+0xff30;
			printf("Wave Pattern Ram: ");
			memdump32((int *)ptr_addr, gddb_buf, 1, 4, 16);
			printf("%s ", gddb_buf);
			memdump32((int *)ptr_addr+4, gddb_buf, 1, 4, 16);
			printf("%s ", gddb_buf);
			memdump32((int *)ptr_addr+8, gddb_buf, 1, 4, 16);
			printf("%s ", gddb_buf);
			memdump32((int *)ptr_addr+12, gddb_buf, 1, 4, 16);
			printf("%s\n", gddb_buf);
		}
		else
			printf("Bad argument: %s\n", ptr_ptrs[1]);
	}
}

void
gddb_step(int num_args, char **ptr_ptrs)
{
	if (num_args>2)
		printf("Usage: step\n");
	else
		gddb_loop=0;
	if (num_args==2)
		gddb_tmp = atoi(ptr_ptrs[1]);
}

void
gddb_help(int num_args, char **ptr_ptrs)
{
	if (num_args > 1)
		printf("Usage: help\n");
	else
		printf("Commands: step, show, print, break, disasm\nPress enter to step...\n");
}
