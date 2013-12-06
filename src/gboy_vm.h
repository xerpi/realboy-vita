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
/* Addresses */
#define NIN_LOG 0x104 // Nintendo logo starts
#define GAM_TIT 0x134 // ASCII game title
#define MAN_COD 0x13f // Manufacurer code
#define CGB_FLG 0x143 // CGB support
#define LIC_NEW 0x144 // Licensee (new)
#define SGB_FLG 0x146 // SGB support
#define CAR_TYP 0x147 // Memory sets supported by cartridge
#define ROM_SIZ 0x148 // ROM size in (32KB<<n) units
#define RAM_SIZ 0x149 // External RAM size
#define JAP_VER 0x14a // Japanese version
#define GAM_LIC 0x14b // Licensee

/* Some sizes */
#define CART_HDR 336 // Size of header
#define LOG_SIZ 29 // Size of Nintendo logo

/* Some external definitions */
/* Defined in gboy_x86_64.S or gboy_cpu.c depending on which core is used */
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
extern long gb_line_clks;
extern long gb_vbln_clks;
extern long gb_oam_clks;
extern long gb_vram_clks;
extern long gb_hblank_clks;
extern long addr_sp_ptrs[0x10];

/* Defined in globals.c */
extern void vid_start();
extern void vid_reset();
extern void snd_reset();
extern void snd_start();
extern void gddb_reset();
extern FILE *open_save_try(char *);
extern FILE *create_save(char *);
extern void rom_exec(int);
extern void rom_exec(int);
extern void mbc_init(int);
extern Uint32 gboy_mode;
extern Uint8 addr_sp[];
extern int use_boot_rom;
extern char *file_path;

/* Locally-global variables */
int use_boot_rom=0;
Uint32 gboy_mode=0; // Game Boy/Color Game Boy mode
Uint32 gboy_hw=0; // Game Boy/Color Game Boy hardware
long ints_offs[] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
const char *types_vec[] = { "Rom only", "MBC1", "MBC1+RAM", "MBC1+RAM+BATTERY", NULL, "MBC2", "MBC2+BATTERY", NULL, "ROM+RAM", "ROM+RAM+BATTERY", NULL, "MMM01", "MMM01+RAM", "MMM01+RAM+BATTERY", NULL, "MBC3+TIMER+BATTERY", "MBC3+TIMER+RAM+BATTERY", "MBC3", "MBC3+RAM", "MBC3+RAM+BATTERY", NULL, NULL, NULL, NULL, NULL, "MBC5", "MBC5+RAM", "MBC5+RAM+BATTERY", "MBC5+RUMBLE", "MBC5+RUMBLE+RAM", "MBC5+RUMBLE+RAM+BATTERY" };
char *base_name;
char *save_name;
static const char * const gb_boot_strs[] = { "boot_roms/dmg_boot.bin", "boot_roms/gbc_boot.bin" };
static Uint8 cart_init_rd[336]; // Temporary space for cartridge's header
static const Uint8 nin_log[] = { 0xce, 0xed, 0x66, 0x66,  0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f,  0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,  0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e }; // Nintendo Game Boy signature
static char *ram_sz_vec[] = { "None", "Ram size 2kb", "Ram size 8kb", "Ram size 32kb" };

static char *rom_sz_vec[] = { "Rom size 32kb", "Rom size 64kb", "Rom size 128kb", 
						"Rom size 256kb", "Rom size 512kb", "Rom size 1mb", 
						"Rom size 2mb", "Rom size 4mb" };

