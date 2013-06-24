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
#define CART_HDR 336
#define NIN_LOG 0x104
#define LOG_SIZ 29
#define NUM_SCAN 152
#define GAM_TIT 0x134 // ascii game title
#define MAN_COD 0x13f // manufacurer code
#define CGB_FLG 0x143 // CGB support
#define GAM_LIC_NEW 0x144 // licensee (new)
#define JAP_VER 0x14a // japanese version
#define GAM_LIC 0x14b // licensee
#define SGB_FLG 0x146 // SGB support
#define CAR_TYP 0x147 // memory sets supported by cartridge
#define ROM_SIZ 0x148 // ROM size in (32KB<<n) units
#define RAM_SIZ 0x149 // external RAM size

/* Defined in gboy_cpu.S */
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
extern long addr_sp_ptrs[0x10]; // pointers to address spaces
extern Uint8 addr_sp[]; // pointers to address spaces

int with_boot=0;
const char *gb_boot_strs[] = { "boot_roms/dmg_rom.bin", "boot_roms/gbc_bios.bin" };
/* Temporary space for cartridge's header */
static Uint8 cart_init_rd[336];
/* Nintendo Gameboy signature */
static const unsigned char nin_log[] = { 0xce, 0xed, 0x66, 0x66,  0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f,  0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,  0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e };

void
gboy_reset()
{
	int i;

#ifdef USE_X86_64_ASM
	long *ptr_regs = &regs_sets;

	if (type==0) {
		ptr_regs[0] = 0x01b0;
		ptr_regs[1] = 0x0013;
		ptr_regs[2] = 0x00d8;
		ptr_regs[3] = 0x014d;
	}
	else {
		ptr_regs[0] = 0x11b0;
		ptr_regs[1] = 0x0000;
		ptr_regs[2] = 0xff56;
		ptr_regs[3] = 0x000d;
	}
	ptr_regs[4] = 0xfffe;
	ptr_regs[5] = 0x0100;
#else
	if (type == 0) {
		regs_sets.regs[AF].UWord = 0x01b0;
		regs_sets.regs[BC].UWord = 0x0013;
		regs_sets.regs[DE].UWord = 0x00d8;
		regs_sets.regs[HL].UWord = 0x014d;
	}
	else {
		regs_sets.regs[AF].UWord = 0x11b0;
		regs_sets.regs[BC].UWord = 0x0000;
		regs_sets.regs[DE].UWord = 0xff56;
		regs_sets.regs[HL].UWord = 0x000d;
	}
	regs_sets.regs[SP].UWord = 0xfffe;
	regs_sets.regs[PC].UWord = 0x0100;
#endif
  
	for (i=0xff00; i<0xffff; i++)
	  addr_sp[i] = 0;

	addr_sp[IR_REG] = 1;
	addr_sp[LCDC_REG] = 0x91;
	if (type == 1) {
		addr_sp[0xff68] = 0xc0;
		addr_sp[0xff6a] = 0xc0;
		addr_sp[0xff55] = 0xff;
	}

// if(gbCgbMode) {
//   if(gbSgbMode) {
//     if(gbEmulatorType == 5)
//       AF.W = 0xffb0;
//     else
//       AF.W = 0x01b0;
//     BC.W = 0x0013;
//     DE.W = 0x00d8;
//     HL.W = 0x014d;
//     for(int i = 0; i < 8; i++)
//       gbPalette[i] = systemGbPalette[gbPaletteOption*8+i];
//   } else {
//     AF.W = 0x11b0;
//     BC.W = 0x0000;
//     DE.W = 0xff56;
//     HL.W = 0x000d;
//   }
//   if(gbEmulatorType == 4)
//     BC.B.B1 |= 0x01;
//  
//   register_HDMA5 = 0xff;
//   gbMemory[0xff68] = 0xc0;
//   gbMemory[0xff6a] = 0xc0;    
// } else {
//   for(int i = 0; i < 8; i++)
//     gbPalette[i] = systemGbPalette[gbPaletteOption*8+i];
// }
//
// if(gbSpeed) {
//   gbSpeedSwitch();
//   gbMemory[0xff4d] = 0;
// }
// 
// gbDivTicks = GBDIV_CLOCK_TICKS;
// gbLcdMode = 2;
// gbLcdTicks = GBLCD_MODE_2_CLOCK_TICKS;
// gbLcdLYIncrementTicks = 0;
// gbTimerTicks = 0;
// gbTimerClockTicks = 0;
// gbSerialTicks = 0;
// gbSerialBits = 0;
// gbSerialOn = 0;
// gbWindowLine = -1;
// gbTimerOn = 0;
// gbTimerMode = 0;
// //  gbSynchronizeTicks = GBSYNCHRONIZE_CLOCK_TICKS;
// gbSpeed = 0;
// gbJoymask[0] = gbJoymask[1] = gbJoymask[2] = gbJoymask[3] = 0;
// 
// if(gbCgbMode) {
//   gbSpeed = 0;
//   gbHdmaOn = 0;
//   gbHdmaSource = 0x0000;
//   gbHdmaDestination = 0x8000;
//   gbVramBank = 0;
//   gbWramBank = 1;
//   register_LY = 0x90;
//   gbLcdMode = 1;
//   for(int i = 0; i < 64; i++)
//     gbPalette[i] = 0x7fff;
// }
//
// if(gbSgbMode) {
//   gbSgbReset();
// }
//
// for(int i =0; i < 4; i++)
//   gbBgp[i] = gbObp0[i] = gbObp1[i] = i;
//
// memset(&gbDataMBC1,0, sizeof(gbDataMBC1));
// gbDataMBC1.mapperROMBank = 1;
//
// gbDataMBC2.mapperRAMEnable = 0;
// gbDataMBC2.mapperROMBank = 1;
//
// memset(&gbDataMBC3,0, 6 * sizeof(int));
// gbDataMBC3.mapperROMBank = 1;
//
// memset(&gbDataMBC5, 0, sizeof(gbDataMBC5));
// gbDataMBC5.mapperROMBank = 1;
//
// memset(&gbDataHuC1, 0, sizeof(gbDataHuC1));
// gbDataHuC1.mapperROMBank = 1;
//
// memset(&gbDataHuC3, 0, sizeof(gbDataHuC3));
// gbDataHuC3.mapperROMBank = 1;
//
// gbMemoryMap[0x00] = &gbRom[0x0000];
// gbMemoryMap[0x01] = &gbRom[0x1000];
// gbMemoryMap[0x02] = &gbRom[0x2000];
// gbMemoryMap[0x03] = &gbRom[0x3000];
// gbMemoryMap[0x04] = &gbRom[0x4000];
// gbMemoryMap[0x05] = &gbRom[0x5000];
// gbMemoryMap[0x06] = &gbRom[0x6000];
// gbMemoryMap[0x07] = &gbRom[0x7000];
// if(gbCgbMode) {
//   gbMemoryMap[0x08] = &gbVram[0x0000];
//   gbMemoryMap[0x09] = &gbVram[0x1000];
//   gbMemoryMap[0x0a] = &gbMemory[0xa000];
//   gbMemoryMap[0x0b] = &gbMemory[0xb000];
//   gbMemoryMap[0x0c] = &gbMemory[0xc000];
//   gbMemoryMap[0x0d] = &gbWram[0x1000];
//   gbMemoryMap[0x0e] = &gbMemory[0xe000];
//   gbMemoryMap[0x0f] = &gbMemory[0xf000];        
// } else {
//   gbMemoryMap[0x08] = &gbMemory[0x8000];
//   gbMemoryMap[0x09] = &gbMemory[0x9000];
//   gbMemoryMap[0x0a] = &gbMemory[0xa000];
//   gbMemoryMap[0x0b] = &gbMemory[0xb000];
//   gbMemoryMap[0x0c] = &gbMemory[0xc000];
//   gbMemoryMap[0x0d] = &gbMemory[0xd000];
//   gbMemoryMap[0x0e] = &gbMemory[0xe000];
//   gbMemoryMap[0x0f] = &gbMemory[0xf000];    
// }
//
// if(gbRam) {
//   gbMemoryMap[0x0a] = &gbRam[0x0000];
//   gbMemoryMap[0x0b] = &gbRam[0x1000];
// }
//
// gbSoundReset();
//
// systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
//
// gbLastTime = systemGetClock();
// gbFrameCount = 0;
}

/*
 * Load boot ROM.
 */
static void
ld_boot()
{
	/* Open file XXX */
	if ( (boot_fd = open(gb_boot_strs[type], 0)) == -1)
		printf("Error open\n");
	
	/* Read to address space XXX */
	if (type==1)
		read(boot_fd, addr_sp, 0x8ff);
	else
		read(boot_fd, addr_sp, 256);

}

/*
 * Load boot ROM.
 */
static void
set_up()
{
	/* Start LCD with mode 2 */
	addr_sp[0xff41] |= 0x82;
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
		gb_cart.cart_licensee[0] = (char)cart_init_rd[GAM_LIC_NEW];
		gb_cart.cart_licensee[1] = (char)cart_init_rd[GAM_LIC_NEW+1];
	}
	else {
		gb_cart.cart_licensee[0] = (char)cart_init_rd[GAM_LIC];
		gb_cart.cart_licensee[1] = (char)cart_init_rd[GAM_LIC+1];
	}
	gb_cart.cart_licensee[2] = 0;
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

	/* Set addresses starting with 0x0, 0x1, ..., 0xf to default address space */
	int i;
	for (i=0; i<=16; i++)
		addr_sp_ptrs[i] = (long)addr_sp;

	/* Allocate space for rom banks */
	gb_cart.cart_rom_banks = malloc(0x8000<<gb_cart.cart_rom_size);
	/* Initial ROM addresses */
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
	
	/* If we have external RAM */
	if (gb_cart.cart_ram_size) 
	{
 		/* Allocate space for RAM banks */
		gb_cart.cart_ram_banks = malloc(1024*gb_cart.cart_ram_size);
		/* Try to open RAM file */
		if ((gb_cart.cart_ram_fd=fopen(gb_cart.cart_name, "r+"))==NULL)
		{
			/* There is no RAM file; create one */
			gb_cart.cart_ram_fd = fopen(gb_cart.cart_name, "w+");
			fwrite(gb_cart.cart_ram_banks, 1, 1024*gb_cart.cart_ram_size, gb_cart.cart_ram_fd);
		}
		/* There exists a RAM file, so read it to RAM space */
		else
			fread(gb_cart.cart_ram_banks, 1, 1024*gb_cart.cart_ram_size, gb_cart.cart_ram_fd);
		/* Initial RAM addresses */
		addr_sp_ptrs[0xa]=addr_sp_ptrs[0xb]=(long)(gb_cart.cart_ram_banks-0xa000);
	}
	/* The cartridge has no external RAM */
	else
		gb_cart.cart_ram_banks = NULL;

	/* If CGB, assign second bank of VRAM and WRAM banks */
	if (type==1)
	{
		gb_cart.cart_vram_bank=(char *)malloc(0x2000);
		gb_cart.cart_wram_bank=(char *)malloc(0x1000*7);
		gb_cart.cart_cuvram_bank=0;
		gb_cart.cart_cuwram_bank=0;
	}
	else
	{
		gb_cart.cart_vram_bank=NULL;
		gb_cart.cart_wram_bank=NULL;
	}

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
	addr_sp[LCDC_REG] = 0x0;
	addr_sp[SCY] = 0;
	addr_sp[SCX] = 0;
	addr_sp[LYC_REG] = 0;
	addr_sp[BGP_REG] = 0xfc;
	addr_sp[OBP0_REG] = 0xff;
	addr_sp[OBP1_REG] = 0xff;
	addr_sp[WY_REG] = 0;
	addr_sp[WX_REG] = 0;
	addr_sp[IE_REG] = 0;
	addr_sp[JOY_REG] = 0x0;
	addr_sp[0xffbb] = 0;
}

/*
 * Start Virtual Machine.
 */
int
start_vm()
{
	/* Read cartridge's header */
	read(rom_fd, cart_init_rd, CART_HDR);

	/* Check header */
	int i;
	for (i=0; i<LOG_SIZ; i++) {
		if (((cart_init_rd+NIN_LOG)[i]) != nin_log[i])
			break;
	}

	if (cart_init_rd[CGB_FLG] == 0x80 || cart_init_rd[CGB_FLG] == 0xc0)
		type=1;
	/* 
	 * If valid ROM (XXX this doesn't actually test for the validity of a ROM; 
	 * this is done through checksuming)
	 */
	if (i==LOG_SIZ) {
		/* Get information from cartridge's header */
		parse_cart_hdr();
		/* Allocate and initialize address space */
		alloc_addr_sp();
		/* Initialize LCD timings */
		set_up();
		/* Initialize MBC driver */
		mbc_init(gb_cart.cart_type);
		/* Load additional banks */
		pread(rom_fd, gb_cart.cart_rom_banks, 0x8000<<gb_cart.cart_rom_size, 0x4000);
		/* Initialize the video subsystem */
		vid_start();
		/* Initialize the sound subsystem */
		snd_start();
		/* Load boot rom */
		if (with_boot) {
			ld_boot();
			if (type==0)
				pread(rom_fd, addr_sp+0x100, 0x4000-0x100, 256);
			else
				pread(rom_fd, addr_sp+0x100, 256, 256);
			rom_exec(0);
		}
		else {
			pread(rom_fd, addr_sp, 0x4000, 0);
			gboy_reset();
			rom_exec(0x100);
		}
	}
	/* Else bad ROM */
	else
		return -1;

	/* Free resources */
	free(gb_cart.cart_rom_banks);
	if (gb_cart.cart_ram_size && (gb_cart.cart_ram_banks!=NULL))
	{
		free(gb_cart.cart_ram_banks);
		fclose(gb_cart.cart_ram_fd);
	}
	if (type==1)
	{
		if (gb_cart.cart_vram_bank!=NULL)
			free(gb_cart.cart_vram_bank);
		if (gb_cart.cart_wram_bank!=NULL)
			free(gb_cart.cart_wram_bank);
	}
	close(boot_fd);
	close(rom_fd);
	vid_reset();
	snd_reset();
	gddb_reset();
	SDL_Quit();

	return 0;
}
