/* RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
 * Copyright (C) 1999-2003 Forgotten
 * Copyright (C) 2004 Forgotten and the VBA development team
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

/*
 * XXX
 * Various notes on RTC:
 * - The current implementation assumes that reads/writes are done at address 0xa000; it doesn't seem
 * to hurt, as practically from what I've seen, games always use this address. However, access actually
 * can be done everywhere at the range 0xa000-0xbfff.
 * - Day Counter Carry Bit is not handled.
 */

#include "gboy.h"
extern long addr_sp_ptrs[0x10]; // pointer to address spaces
extern long *addr_sp_bases[0x10]; // pointers to bases
//extern long test_tmp_val;

/*
 * From VisualBoy.
 * XXX GPL; REWRITE.
 */
static
mbc3_latch_rtc()
{
  time_t now = time(NULL);
  time_t diff = now - gb_mbc.mbc_rtc_last;
  if(diff > 0) {
    /* Update the clock according to the last update time */
    gb_mbc.mbc_rtc_regs[0] += diff % 60;
    if(gb_mbc.mbc_rtc_regs[0] > 59) {
      gb_mbc.mbc_rtc_regs[0] -= 60;
      gb_mbc.mbc_rtc_regs[1]++;
    }

    diff /= 60;

    gb_mbc.mbc_rtc_regs[1] += diff % 60;
    if(gb_mbc.mbc_rtc_regs[1] > 60) {
      gb_mbc.mbc_rtc_regs[1] -= 60;
      gb_mbc.mbc_rtc_regs[2]++;
    }

    diff /= 60;

    gb_mbc.mbc_rtc_regs[2] += diff % 24;
    if(gb_mbc.mbc_rtc_regs[2] > 24) {
      gb_mbc.mbc_rtc_regs[2] -= 24;
      gb_mbc.mbc_rtc_regs[3]++;
    }
    diff /= 24;

    gb_mbc.mbc_rtc_regs[3] += diff;
    if(gb_mbc.mbc_rtc_regs[3] > 255) {
      if(gb_mbc.mbc_rtc_regs[3] > 511) {
        gb_mbc.mbc_rtc_regs[3] %= 512;
        gb_mbc.mbc_rtc_regs[4] |= 0x80;
      }
      gb_mbc.mbc_rtc_regs[4] = (gb_mbc.mbc_rtc_regs[4] & 0xfe) |
        (gb_mbc.mbc_rtc_regs[3]>255 ? 1 : 0);
    }
  }
  gb_mbc.mbc_rtc_last = now;
	
}

/*
 * From VisualBoy.
 * XXX GPL; REWRITE.
 */
static
mbc3_1st_rtc()
{
	gb_mbc.mbc_rtc_last = time(NULL);
	struct tm *loc_tim = localtime(&gb_mbc.mbc_rtc_last);

	gb_mbc.mbc_rtc_regs[0] = loc_tim->tm_sec;
	gb_mbc.mbc_rtc_regs[1] = loc_tim->tm_min;
	gb_mbc.mbc_rtc_regs[2] = loc_tim->tm_hour;
	gb_mbc.mbc_rtc_regs[3] = loc_tim->tm_yday & 255;
	gb_mbc.mbc_rtc_regs[4] = (gb_mbc.mbc_rtc_regs[4] & 0xfe) | (loc_tim->tm_yday>255 ? 1 : 0);
}

void
mbc3_read_rtc()
{
	char *rtc_fname;

	rtc_fname = malloc(500);//strnlen(gb_cart.cart_name+5, 255));
	strncpy(rtc_fname, gb_cart.cart_name, 255);
	strncat(rtc_fname, ".rtc", 255);

	/* Try to open RTC file */
	if ((gb_cart.cart_rtc_fd = fopen(rtc_fname, "r+")) == NULL) {
		/* There is no RTC file; create one */
		gb_cart.cart_rtc_fd = fopen(rtc_fname, "w+");
		fwrite(&gb_mbc.mbc_rtc_last, 1, 5+(sizeof(time_t)), gb_cart.cart_rtc_fd);
		mbc3_1st_rtc();
	}
	/* There exists a RTC file, so read it to RTC space */
	else
		fread(&gb_mbc.mbc_rtc_last, 1, 5+(sizeof(time_t)), gb_cart.cart_rtc_fd);
}

void
mbc3_ram_remap()
{
	if (!(gb_mbc.mbc_rtc_reg_sel)) {
		addr_sp_ptrs[0xa]=addr_sp_ptrs[0xb]=((long)((&gb_cart.cart_ram_banks[0x2000*gb_cart.cart_curam_bank])-0xa000));
	}
	else {
		addr_sp_ptrs[0xa]=addr_sp_ptrs[0xb]=((long)((&gb_mbc.mbc_rtc_regs[gb_mbc.mbc_rtc_reg_sel-8])-0xa000));
	}
}

void
mbc3_clk(int val)
{
	if (gb_mbc.mbc_rtc_latch==0 && val==1)
		mbc3_latch_rtc();
	if (val==0 || val==1)
		gb_mbc.mbc_rtc_latch=val;
}

void
mbc3_ramrtc_bank(int val)
{
	if (val >= 8)
		gb_mbc.mbc_rtc_reg_sel = val&0x0f;
	else {
		gb_mbc.mbc_rtc_reg_sel = (char)0; 
		gb_cart.cart_curam_bank = val;
	}

	mbc3_ram_remap();
}

void
mbc3_rom_bank(int val)
{
	if (val==0)
		gb_cart.cart_curom_bank = 1; // new ROM bank
	else
		gb_cart.cart_curom_bank = val&0x7f; // new ROM bank

	mbc_rom_remap(); // remap
}

void
mbc3_ramtim_en(int val)
{
	/* Sync RAM file */
	if ((val&=0xf) != 0xa) {
		rewind(gb_cart.cart_ram_fd);
		fwrite(gb_cart.cart_ram_banks, 1, 1024*gb_cart.cart_ram_size, gb_cart.cart_ram_fd);
		if (gb_cart.cart_rtc_fd != NULL) {
				rewind(gb_cart.cart_rtc_fd);
				fwrite(&gb_mbc.mbc_rtc_last, 1, 5+(sizeof(time_t)), gb_cart.cart_rtc_fd);
		}
	}
}
