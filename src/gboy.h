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

#include <ctype.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <SDL/SDL.h>
#include <termios.h>

/* Memory-mapped IO addresses */
#define JOY_REG 0xff00 // address for joypad register
#define TIMA 0xff05 // address for tima register
#define TMA 0xff06 // address for tma register
#define TAC 0xff07 // address for tac register
#define NR10 0xff10 // address for NR10 register
#define NR11 0xff11 // address for NR11 register
#define NR12 0xff12 // address for NR12 register
#define NR13 0xff13 // address for NR13 register
#define NR14 0xff14 // address for NR14 register
#define NR20 0xff15 // address for NR20 register
#define NR21 0xff16 // address for NR21 register
#define NR22 0xff17 // address for NR22 register
#define NR23 0xff18 // address for NR23 register
#define NR24 0xff19 // address for NR24 register
#define NR30 0xff1a // address for NR30 register
#define NR31 0xff1b // address for NR31 register
#define NR32 0xff1c // address for NR32 register
#define NR33 0xff1e // address for NR33 register
#define NR34 0xff1f // address for NR34 register
#define NR41 0xff20 // address for NR41 register
#define NR42 0xff21 // address for NR42 register
#define NR43 0xff22 // address for NR43 register
#define NR44 0xff23 // address for NR44 register
#define NR50 0xff24 // address for NR50 register
#define NR51 0xff25 // address for NR51 register
#define NR52 0xff26 // address for NR52 register
#define LCDC_REG 0xff40 // address for lcd control register
#define LCDS_REG 0xff41 // address for lcd status register
#define SCY 0xff42 // address for lcd control register
#define SCX 0xff43 // address for lcd control register
#define LY_REG 0xff44 // address for ly control register
#define LYC_REG 0xff45 // address for ly control register
#define BGP_REG 0xff47 // address for ly control register
#define OBP0_REG 0xff48 // address for ly control register
#define OBP1_REG 0xff49 // address for ly control register
#define WY_REG 0xff4a // address for ly control register
#define WX_REG 0xff4b // address for ly control register
#define IR_REG 0xff0f // address for interrupt request register
#define IE_REG 0xffff // address for interrupt enable register

#ifdef USE_X86_64_ASM
	/* General-purpose registers offsets */
	#define F 0 // flags
	#define A 1 // accumulator
	#define C 8 // general
	#define B 9 // general
	#define E 16 // general
	#define D 17 // general
	#define L 24 // general
	#define H 25 // general
	/* Register pairs offsets */
	#define AF 0
	#define BC 8
	#define DE 16
	#define HL 24
	/* Stack Pointer and Program Counter registers offsets */
	#define SP 32
	#define PC 40
#else
	/* General-purpose registers offsets */
	#define F 0 // flags
	#define A 1 // accumulator
	#define C 2 // general
	#define B 3 // general
	#define E 4 // general
	#define D 5 // general
	#define L 6 // general
	#define H 7 // general
	/* Register pairs offsets */
	#define AF 0
	#define BC 2
	#define DE 4
	#define HL 6
	/* Stack Pointer and Program Counter registers offsets */
	#define SP 8
	#define PC 10
#endif


/* Addressing bytes or words */
#define BYTE_B 0xffffff00
#define WORD_W 0xffff0000

/* Addressing modes */
#define IMP 0x01
#define REG 0x02
#define REG_IND 0x04
#define IMM 0x08
#define IMM_IND 0x10

/* Key presses */
#define RET_MASK 0x1
#define D_MASK 0x2
#define S_MASK 0x4
#define A_MASK 0x8
#define UP_MASK 0x10
#define DOWN_MASK 0x20
#define LEFT_MASK 0x40
#define RIGHT_MASK 0x80

/* Misc */
#define SCALE 0
#define FULLSCREEN 1
#define FPS 2
#define BOOT 3
#define GB_MODE 4
#define READ 0
#define WRITE 1
#define MAX_STRS 8 // maximum strings
#define NUM_CMDS 6 // number of commands in CLI interface
#define AUTO -1
#define DMG 0
#define CGB 1
#define SGB 2

/*
 * Structures used by RealBoy
 */

/* Information about the cartridge being executed */
struct gb_cart {
	char cart_name[17];
	char cart_licensee[3];
	Uint8 cart_type; // MBC used by cartridge
	Uint8 cart_rom_size;
	Uint8 cart_ram_size;
	Uint8 cart_sgb; // cartridge supports SGB
	Uint8 cart_cgb; // cartridge supports CGB
	Uint32 cart_curom_bank;
	Uint32 cart_curam_bank;
	Uint32 cart_cuvram_bank;
	Uint32 cart_cuwram_bank;
	Uint8 *cart_rom_banks;
	Uint8 *cart_ram_banks;
	Uint8 *cart_vram_bank;
	Uint8 *cart_wram_bank;
	FILE *cart_ram_fd;
	FILE *cart_rtc_fd; // MBC3's Real Time Clock
} gb_cart;

/* 
 * Information about the cartridge's MBC chip.
 * Do not change unless you know what you're doing;
 * assembly code use this structure, and thus is sensible 
 * to compiler-generated padding. Be careful if adding, removing
 * or changing order of elements. YOU HAVE BEEN WARNED.
 */
struct gb_mbc {
	void (*mbc_funcs[10])(int); // MBC generic functions
	time_t mbc_rtc_last;
	long mbc_rtc_regs[5]; // MBC3 RTC's registers
	char mbc_rtc_latch;
	char mbc_ram_rom_mode;
	char mbc_ram_rom_upp;
	char mbc_rtc_reg_sel;
} gb_mbc;

/* 
 * Structures used by the commmand-line interpreter.
 */

/* Doubly-linked list of commands already executed */
struct cmd_stack {
	char *cmd_buf;
	void (*cmd_fun)(int, char **);
	int num_args;
	char **cmd_ptrs;
	struct cmd_stack *ptr_fw;
	struct cmd_stack *ptr_bk;
};

/* Pointers to beginning, end and current character of input buffer */
struct cmd_line {
	char *ptr_cur; // pointer to current byte in buffer
	char *ptr_end; // pointer at the end of the buffer
	char *ptr_beg; // pointer at the beginning of buffer
} cmd_line;

extern long nb_spr;
/*
 * Global variables defined in globals.c
 * Comments also in globals.c
 */
/* CLI-related globals */
extern char inp_buf[512];
extern char *ptr_dup;
extern char *cmd_ptrs[MAX_STRS+2];
extern char *ram_sz_vec[];
extern char *rom_sz_vec[];
/* Video-related globals */
extern Uint32 pal_grey[4];
extern Uint32 pal_color[32*32*32];
extern Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
extern Uint8 sgb_pal_map[20][18];
extern SDL_Surface *screen;
extern SDL_Surface *back;
extern SDL_Surface *sgb_buf;
extern SDL_Surface *sgb_buf_back;
/* Debug-related globals */
extern int gbddb;
extern int gbplay;
/* Sound-related globals */
extern SDL_AudioSpec desired;
extern Sint16 *playbuf;
extern double freq_tbl[2048];
extern double freq_tbl_snd3[2048];
extern double freq_tbl_snd4[256];
/* Misc globals */
extern FILE *boot_file;
extern Uint32 gboy_mode;
extern Uint32 gboy_hw;
extern FILE *rom_file;
extern Uint8 addr_sp[];
extern long ints_offs[];
extern const char *types_vec[];
extern Uint32 gb_clk_rate;
