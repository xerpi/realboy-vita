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
extern void rom_exec(int);
extern void mbc_init(int);
extern Uint32 gboy_mode;
extern Uint8 addr_sp[];
extern int use_boot_rom;
extern char *file_path;

/* Locally-global variables */
char *base_name;
char *save_name;
static const char * const gb_boot_strs[] = { "boot_roms/dmg_rom.bin", "boot_roms/gbc_bios.bin" };
static Uint8 cart_init_rd[336]; // Temporary space for cartridge's header
static const Uint8 nin_log[] = { 0xce, 0xed, 0x66, 0x66,  0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f,  0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,  0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e }; // Nintendo Game Boy signature


/*
 * Load boot ROM.
 */
static int
ld_boot()
{
	/* Open file XXX */
	if ( (boot_file = fopen(gb_boot_strs[gboy_mode], "r")) == NULL) {
		perror("Error open\n");
		return -1;
	}
	
	/* Read to address space XXX */
	if (gboy_mode==1)
		fread(addr_sp, 1, 0x8ff, boot_file);
	else
		fread(addr_sp, 1, 256, boot_file);

	return 0;
}

/*
 * Set initial state for Virtual Machine.
 * Used when boot ROM is not executed.
 */
static void
gboy_setup()
{
	int i;

	/* Set CPU registers' initial values */
#ifdef USE_X86_64_ASM
	long *ptr_regs = &regs_sets;

	if (gboy_mode==CGB) {
		ptr_regs[0] = 0x11b0;
		ptr_regs[1] = 0x0000;
		ptr_regs[2] = 0xff56;
		ptr_regs[3] = 0x000d;
	}

	else {
		ptr_regs[0] = 0x01b0;
		ptr_regs[1] = 0x0013;
		ptr_regs[2] = 0x00d8;
		ptr_regs[3] = 0x014d;
	}
	ptr_regs[4] = 0xfffe;
	ptr_regs[5] = 0x0100;
#else
	if (gboy_mode==CGB) {
		regs_sets.regs[AF].UWord = 0x11b0;
		regs_sets.regs[BC].UWord = 0x0000;
		regs_sets.regs[DE].UWord = 0xff56;
		regs_sets.regs[HL].UWord = 0x000d;
	}
	else {
		regs_sets.regs[AF].UWord = 0x01b0;
		regs_sets.regs[BC].UWord = 0x0013;
		regs_sets.regs[DE].UWord = 0x00d8;
		regs_sets.regs[HL].UWord = 0x014d;
	}
	regs_sets.regs[SP].UWord = 0xfffe;
	regs_sets.regs[PC].UWord = 0x0100;
#endif
  
	/* Clear memory range: IO registers and high RAM */
	for (i=0xff00; i<0xffff; i++)
	  addr_sp[i] = 0;


	if (gboy_mode == CGB) {
		addr_sp[0xff4d] = 0;
		addr_sp[0xff68] = 0xc0;
		addr_sp[0xff6a] = 0xc0;
		addr_sp[0xff55] = 0xff;
	}
	/* Start LCD with mode 2 */
	addr_sp[0xff41] |= 0x82;

	/* Set some initial values; don't assume boot ROM will set them */
	addr_sp[TIMA] = 0;
	addr_sp[TMA] = 0;
	addr_sp[TAC] = 0;
	addr_sp[NR10] = 0x80;
	addr_sp[NR11] = 0xbf;
	addr_sp[NR12] = 0xf3;
	addr_sp[NR14] = 0xbf;
	addr_sp[NR21] = 0x3f;
	addr_sp[NR22] = 0x00;
	addr_sp[NR24] = 0xbf;
	addr_sp[NR30] = 0x7f;
	addr_sp[NR31] = 0xff;
	addr_sp[NR32] = 0x9f;
	addr_sp[NR33] = 0xbf;
	addr_sp[NR41] = 0xff;
	addr_sp[NR42] = 0;
	addr_sp[NR43] = 0;
	addr_sp[NR30] = 0xbf;
	addr_sp[NR50] = 0x77;
	addr_sp[NR51] = 0xf3;
	addr_sp[NR52] = 0x80;
	addr_sp[LCDS_REG] = 0x80;
	addr_sp[LCDC_REG] = 0x91;
	addr_sp[SCY] = 0;
	addr_sp[SCX] = 0;
	addr_sp[LYC_REG] = 0;
	addr_sp[BGP_REG] = 0xfc;
	addr_sp[OBP0_REG] = 0xff;
	addr_sp[OBP1_REG] = 0xff;
	addr_sp[WY_REG] = 0;
	addr_sp[WX_REG] = 0;
	addr_sp[IE_REG] = 0;
	addr_sp[IR_REG] = 1;
	addr_sp[JOY_REG] = 0x0;
	addr_sp[0xffbb] = 0;
}

/*
 * Parse cartridge information and print it.
 */
static void
parse_cart_hdr()
{
	int i, j;

	/* Copy name XXX limit ourselves to 11 characters; this restriction apparently came with the CGB */
	for (i=0; i<16; i++)
		gb_cart.cart_name[i] = (cart_init_rd+GAM_TIT)[i];

	printf("\nCartridge \"%s\":\n", gb_cart.cart_name);
	printf("================================\n");

	if (cart_init_rd[GAM_LIC]==0x33) {
		gb_cart.cart_licensee[0] = (char)cart_init_rd[LIC_NEW];
		gb_cart.cart_licensee[1] = (char)cart_init_rd[LIC_NEW+1];
	}
	else {
		gb_cart.cart_licensee[0] = (char)cart_init_rd[GAM_LIC];
		gb_cart.cart_licensee[1] = (char)cart_init_rd[GAM_LIC+1];
	}

	gb_cart.cart_licensee[2] = 0; // End string with NULL

	if ( ((gb_cart.cart_sgb |= cart_init_rd[SGB_FLG] & 0x03) == 0x03))
		printf("SGB Support\n");
	
	/* Copy type, ROM size and RAM size */
	gb_cart.cart_type = cart_init_rd[CAR_TYP];
	gb_cart.cart_rom_size = cart_init_rd[ROM_SIZ];
	gb_cart.cart_ram_size = cart_init_rd[RAM_SIZ];

	/* Print information */
	printf("%s\n", types_vec[gb_cart.cart_type&0xff]);
	printf("%s\n", rom_sz_vec[gb_cart.cart_rom_size&0xff]);
	printf("%s\n", ram_sz_vec[gb_cart.cart_ram_size&0xff]);
	if (cart_init_rd[GAM_LIC]==0x33)
		printf("Licensee code: %s\n", gb_cart.cart_licensee);
	else
		printf("Licensee code: 0x%X\n", (Uint8)*gb_cart.cart_licensee);
	if (cart_init_rd[JAP_VER]==0)
		printf("Japanese version\n");
	else
		printf("Non-japanese version\n");

	/* Compute checksum */
	for (i=0x134, j=0; i<=0x14c; i++)
		j=j-cart_init_rd[i]-1;

	/* Verify checksum */
	printf("Checksum: ");
	if ((Uint8)j != cart_init_rd[0x14d])
		printf("Failed!\n");
	else
		printf("OK\n");
}

/*
 * Allocate space according to cartridge's needs.
 */
static void
alloc_addr_sp()
{
	/* Map addresses starting with 0x0, 0x1, ..., 0xf to default address space */
	int i;
	for (i=0; i<=16; i++)
		addr_sp_ptrs[i] = (long)addr_sp;

	/* Allocate space for rom banks */
	gb_cart.cart_rom_banks = malloc(0x8000<<gb_cart.cart_rom_size);

	/* Map address range 0x4000-0x7fff to first ROM bank */
	addr_sp_ptrs[4]=addr_sp_ptrs[5]=addr_sp_ptrs[6]=addr_sp_ptrs[7]=(long)(gb_cart.cart_rom_banks-0x4000);

	/* Determine RAM size: gb_cart.cart_ram_size*1024 bytes */
	switch (gb_cart.cart_ram_size) {
		case 0:
			gb_cart.cart_ram_size = 0;
			break;
		case 1:
			gb_cart.cart_ram_size = 2;
			break;
		case 2:
			gb_cart.cart_ram_size = 8;
			break;
		case 3:
			gb_cart.cart_ram_size = 32;
			break;
		default:
			;
	}

	/* If CGB, assign second bank of VRAM and WRAM banks */
	if (gboy_mode==CGB) {
		gb_cart.cart_vram_bank=(char *)malloc(0x2000);
		gb_cart.cart_wram_bank=(char *)malloc(0x1000*7);
		gb_cart.cart_cuvram_bank=0;
		gb_cart.cart_cuwram_bank=0;
	}
	else {
		gb_cart.cart_vram_bank=NULL;
		gb_cart.cart_wram_bank=NULL;
	}
	
	/* If we have external RAM */
	if (gb_cart.cart_ram_size) 
	{
		base_name = basename(file_path);
		save_name = (char *)malloc(strnlen(base_name, 255)+4);

		strncpy(save_name, base_name, strnlen(base_name, 255));

		/*
		 * Search the filename string for a '.' character.
		 *
		 * Generally ROMS are appended a ".gb" or ".gbc" (e.g. Foo.gbc).
		 * In this case we replace ".gbc" with ".sav" and create the
		 * file in the configuration directory (e.g. Foo.sav).
		 *
		 * However, extensions such as ".gbc" are not necessary; it is
		 * possible to execute a ROM with filename "FooBar". In this case,
		 * no '.' character will be found, and the ".sav" string will just 
		 * be appended to the filename, (FooBar.sav).
		 */
		for (i=0; save_name[i] != '.' && save_name[i] != '\0'; i++)
			;

		/* 
		 * If a '.' character was found, most likely the ROM's filename
		 * is something like "Foo.gb" or "Foo.gbc".
		 * Note that it is possible as well to have a filename "Foo.Bar.Bob",
		 * and this won't cause any trouble (savefile would be "Foo.sav").
		 */
		if (save_name[i] == '.')
			i++;
		/* 
		 * If a '.' character was NOT found, we are at the NULL character (end of string).
		 * We have enough space to append ".sav" to the string.
		 */
		else
			save_name[i++] = '.';
		strncpy(save_name+i, "sav", 5);

 		/* Allocate space for RAM banks */
		gb_cart.cart_ram_banks = malloc(1024*gb_cart.cart_ram_size);
		/* Try to open RAM file */
		if ((gb_cart.cart_ram_fd=fopen(save_name, "r+"))==NULL)
		{
			/* There is no RAM file; create one */
			gb_cart.cart_ram_fd = fopen(save_name, "w+");
			fwrite(gb_cart.cart_ram_banks, 1, 1024*gb_cart.cart_ram_size, gb_cart.cart_ram_fd);
		}
		/* There exists a RAM file, so read it to RAM space */
		else
			fread(gb_cart.cart_ram_banks, 1, 1024*gb_cart.cart_ram_size, gb_cart.cart_ram_fd);
		/* Initial RAM addresses */
		addr_sp_ptrs[0xa]=addr_sp_ptrs[0xb]=(long)(gb_cart.cart_ram_banks-0xa000);

		free(save_name);
	}
	/* The cartridge has no external RAM */
	else
		gb_cart.cart_ram_banks = NULL;
}

/*
 * Start Virtual Machine.
 */
int
start_vm()
{
	/* Read cartridge's header */
	fread(cart_init_rd, 1, CART_HDR, rom_file);

	/* Check header */
	int file_logo_size;
	for (file_logo_size=0; file_logo_size<LOG_SIZ; file_logo_size++) {
		if (((cart_init_rd+NIN_LOG)[file_logo_size]) != nin_log[file_logo_size])
			break;
	}

	/* Set CGB mode if Game Boy Color supported by cartridge */
	if (cart_init_rd[CGB_FLG] == 0x80 || cart_init_rd[CGB_FLG] == 0xc0)
		gboy_mode=CGB;

	/* 
	 * If valid ROM (XXX this doesn't actually test for the validity of a ROM; 
	 * this is done through checksuming)
	 */
	if (file_logo_size==LOG_SIZ)
	{
		/* Get information from cartridge's header */
		parse_cart_hdr();
		/* Allocate and initialize address space */
		alloc_addr_sp();
		/* Initialize MBC driver */
		mbc_init(gb_cart.cart_type);
		/* Load additional banks */
		fseek(rom_file, 0x4000, SEEK_SET);
		fread(gb_cart.cart_rom_banks, 1, 0x8000<<gb_cart.cart_rom_size, rom_file);
		/* Initialize the video subsystem */
		vid_start();
		/* Initialize the sound subsystem */
		snd_start();
		/* Load boot rom */
		if (use_boot_rom) {
			/* If error loading boot ROM, just fallback to boot without it */	
			if (ld_boot() == -1) {
				gboy_setup();
				rewind(rom_file);
				fread(addr_sp, 1, 0x4000, rom_file);
				rom_exec(0x100);
			}
			else {
				if (gboy_mode==CGB) {
					fseek(rom_file, 256, SEEK_SET);
					fread(addr_sp+0x100, 1, 256, rom_file);
				}
				else {
					fseek(rom_file, 256, SEEK_SET);
					fread(addr_sp+0x100, 1, 0x4000-0x100, rom_file);
				}
				rom_exec(0);
			}
		}
		else {
			gboy_setup();
			rewind(rom_file);
			fread(addr_sp, 1, 0x4000, rom_file);
			rom_exec(0x100);
		}
	}
	/* Else bad ROM; return error and exit */
	else
		return -1;

	/* Free resources */
	free(gb_cart.cart_rom_banks);
	if (gb_cart.cart_ram_size && (gb_cart.cart_ram_banks!=NULL))
	{
		free(gb_cart.cart_ram_banks);
		fclose(gb_cart.cart_ram_fd);
	}
	if (gboy_mode==1)
	{
		if (gb_cart.cart_vram_bank!=NULL)
			free(gb_cart.cart_vram_bank);
		if (gb_cart.cart_wram_bank!=NULL)
			free(gb_cart.cart_wram_bank);
	}

	/* Release resources */
	if (use_boot_rom)
		fclose(boot_file);
	fclose(rom_file);
	vid_reset();
	snd_reset();
	gddb_reset();
	SDL_Quit();

	return 0;
}
