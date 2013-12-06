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
extern long addr_sp_ptrs[0x10]; // pointer to address spaces

/* Functions defined in gboy_mbcX.c */
extern void mbc1_mode(int);
extern void mbc1_ram_bank(int);
extern void mbc1_rom_bank(int);
extern void mbc1_ram_en(int);
extern void mbc2_rom_bank(int);
extern void mbc2_ram_en(int);
extern void mbc2_ram_wr(int);
extern void mbc3_clk(int);
extern void mbc3_read_rtc();
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
