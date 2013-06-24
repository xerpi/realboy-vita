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
extern long addr_sp_ptrs[0x10]; // pointer to address spaces

/* Functions included in various mbc's */
extern void mbc1_mode(int);
extern void mbc1_ram_bank(int);
extern void mbc1_rom_bank(int);
extern void mbc1_ram_en(int);
extern void mbc2_rom_bank(int);
extern void mbc2_ram_en(int);
extern void mbc2_ram_wr(int);
extern void mbc3_clk(int);
extern void mbc3_ramrtc_rdwr(int, int);
extern void mbc3_ramrtc_bank(int);
extern void mbc3_rom_bank(int);
extern void mbc3_ramtim_en(int);
extern void mbc5_mode(int);
extern void mbc5_ram_bank(int);
extern void mbc5_rom_bank_low(int);
extern void mbc5_rom_bank_high(int);
extern void mbc5_ram_en(int);
extern void mbc5_dummy(int);

/*
 * General RAM remap function.
 */
void
mbc_ram_remap()
{
	addr_sp_ptrs[0xa] = addr_sp_ptrs[0xb] = ((long)((&gb_cart.cart_ram_banks[0x2000*gb_cart.cart_curam_bank])-0xa000));
}

/*
 * General ROM remap function.
 */
void
mbc_rom_remap()
{
	addr_sp_ptrs[7] = addr_sp_ptrs[4] = addr_sp_ptrs[5] = addr_sp_ptrs[6] = ((long)((&gb_cart.cart_rom_banks[0x4000*(gb_cart.cart_curom_bank-1)])-0x4000));
}

/*
 * Main structure holding the various mbcX-specific functions.
 * Used by Assembly routine gboy_cpu.S
 */
static void (*mbc_def_funcs[4][5])(int) = { mbc1_ram_en, mbc1_rom_bank, mbc1_ram_bank, mbc1_mode, NULL, mbc2_ram_wr, mbc2_ram_en, mbc2_rom_bank, NULL, NULL, mbc3_ramtim_en, mbc3_rom_bank, mbc3_ramrtc_bank, mbc3_clk, NULL, mbc5_ram_en, mbc5_rom_bank_low, mbc5_rom_bank_high, mbc5_ram_bank, mbc5_dummy };

/*
 * Initialize gb_mbc structure.
 */
void
mbc_init(int mbc_num)
{

	switch (mbc_num) {
		case 1:
		case 2:
		case 3:
			mbc_num = 0;
			break;
		case 5:
		case 6:
			mbc_num = 1;
			break;
		case 0x0f:
		case 0x10:
			mbc3_read_rtc();
		case 0x11:
		case 0x12:
		case 0x13:
			mbc_num = 2;
			break;
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
			mbc_num = 3;
			break;
	};

	if (mbc_num==3)
	{
		gb_mbc.mbc_funcs[0] = mbc_def_funcs[mbc_num][0];
		gb_mbc.mbc_funcs[1] = mbc_def_funcs[mbc_num][0];
		gb_mbc.mbc_funcs[2] = mbc_def_funcs[mbc_num][1];
		gb_mbc.mbc_funcs[3] = mbc_def_funcs[mbc_num][2];
		gb_mbc.mbc_funcs[4] = mbc_def_funcs[mbc_num][3];
		gb_mbc.mbc_funcs[5] = mbc_def_funcs[mbc_num][3];
		gb_mbc.mbc_funcs[6] = mbc_def_funcs[mbc_num][4];
		gb_mbc.mbc_funcs[7] = mbc_def_funcs[mbc_num][4];
	}
	
	else
	{
		gb_mbc.mbc_funcs[0] = mbc_def_funcs[mbc_num][0];
		gb_mbc.mbc_funcs[1] = mbc_def_funcs[mbc_num][0];
		gb_mbc.mbc_funcs[2] = mbc_def_funcs[mbc_num][1];
		gb_mbc.mbc_funcs[3] = mbc_def_funcs[mbc_num][1];
		gb_mbc.mbc_funcs[4] = mbc_def_funcs[mbc_num][2];
		gb_mbc.mbc_funcs[5] = mbc_def_funcs[mbc_num][2];
		gb_mbc.mbc_funcs[6] = mbc_def_funcs[mbc_num][3];
		gb_mbc.mbc_funcs[7] = mbc_def_funcs[mbc_num][3];
		gb_mbc.mbc_funcs[8] = mbc_def_funcs[mbc_num][4];
		gb_mbc.mbc_funcs[9] = mbc_def_funcs[mbc_num][4];
	}
}
