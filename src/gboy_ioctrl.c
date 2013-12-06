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
#include "gboy_ioctrl.h"

static void
write_sgb_packet(Uint8 val)
{
	do_sgb_packet(val);
}

static void
write_ly_reg(Uint8 val)
{
	addr_sp[0xff45] = val;
	if (addr_sp[0xff44] == addr_sp[0xff45])
		addr_sp[0xff41]|=4;
}

static void
write_hdma1(Uint8 val)
{
	if (val > 0x7f && val < 0xa0)
		val=0;
	addr_sp[0xff51] = val;
}

static void
write_hdma2(Uint8 val)
{
	addr_sp[0xff52] = val&0xf0;
}

static void
write_hdma3(Uint8 val)
{
	addr_sp[0xff53] = val&0x1f;
}

static void
write_hdma4(Uint8 val)
{
	addr_sp[0xff54] = val&0xf0;
}

static void
write_spr_pal(Uint8 val)
{
	do_spr_pal_wr(val);
}

static void
write_back_pal(Uint8 val)
{
	do_back_pal_wr(val);
}

static void
write_spr(Uint8 val)
{
	do_spr_pal(val);
}

static void
write_back(Uint8 val)
{
	do_back_pal(val);
}

static void
do_remap_wram(Uint8 val)
{
	long *ptr_addr_sp = (long *)&addr_sp_ptrs;
	int temp_val = val&0x7;

	if (temp_val==0 || temp_val==1)
		ptr_addr_sp[0xd] = (long)(addr_sp);
	else
		ptr_addr_sp[0xd] = (long)((gb_cart.cart_wram_bank+((val-2)*4096))-0xd000);
}

static void
do_remap_vram(Uint8 val)
{
	long *ptr_addr_sp = (long *)&addr_sp_ptrs;

	if ((val&1))
		ptr_addr_sp[8] = ptr_addr_sp[9] = (long)(gb_cart.cart_vram_bank-0x8000);
	else
		ptr_addr_sp[8] = ptr_addr_sp[9] = (long)addr_sp;
}

static void
cgb_speed_switch(Uint8 val)
{
	addr_sp[0xff4d] = (addr_sp[0xff4d]&0x80) | (val&0x1);
}

/*
 * Game Boy Color only.
 */
static void
write_vram_dma(Uint8 val)
{
	do_vram_dma(val);
}

static void
do_oam_dma(unsigned int val)
{
	long *ptr_addr_ptrs = (long *)&addr_sp_ptrs;
	Uint8 *ptr_src;
	Uint8 *ptr_dst;
	int val_offs, i;

	val &= 0xff;
	val <<= 8; // scale
	val_offs = (val>>12); // offset to pointers
	ptr_src = (Uint8 *)(ptr_addr_ptrs[val_offs]+val);
	ptr_dst = addr_sp + 0xfe00; // OAM

	/* Copy stream (XXX copy word at a time) */
	for (i=0; i<160; i++)
		ptr_dst[i] = ptr_src[i];
}
static void
joy_update(Uint8 reg_new)
{
	addr_sp[0xff00] = reg_new|0xcf;

	if (!(reg_new&0x10)) {
		if (key_bitmap&UP_MASK)
		{
			addr_sp[0xff00]&=~4;
		}
		if (key_bitmap&DOWN_MASK)
		{
			addr_sp[0xff00]&=~8;
		}
		if (key_bitmap&LEFT_MASK)
		{
			addr_sp[0xff00]&=~2;
		}
		if (key_bitmap&RIGHT_MASK)
		{
			addr_sp[0xff00]&=~1;
		}
	}
	
	if (!(reg_new&0x20)) {
		if (key_bitmap&A_MASK)
		{
			addr_sp[0xff00]&=~4;
		}
		if (key_bitmap&RET_MASK)
		{
			addr_sp[0xff00]&=~8;
		}
		if (key_bitmap&S_MASK)
		{
			addr_sp[0xff00]&=~2;
		}
		if (key_bitmap&D_MASK)
		{
			addr_sp[0xff00]&=(Uint8)(~1);
		}
	}
}

static void
div_reset()
{
	*(addr_sp+0xff04) = 0; // reset DIV
}

static void
tac_update(int new_val)
{
	switch (new_val&3) {
		case 0:
			cpu_state.tac_counter = 64;
			break;
		case 1:
			cpu_state.tac_counter = 1;
			break;
		case 2:
			cpu_state.tac_counter = 4;
			break;
		case 3:
			cpu_state.tac_counter = 16;
			break;
	}

	cpu_state.tac_reload=cpu_state.tac_counter;
	if (new_val&0x4)
		cpu_state.tac_on=1;
	else
		cpu_state.tac_on=0;
}

static void
disable_boot()
{
	rewind(rom_file);

	/* Read to address space */
	if ((fread(addr_sp, 1, 256, rom_file))==-1)
		perror("fread()");

	if (gboy_mode==1) {
		fseek(rom_file, 256, SEEK_SET);
		if ((fread(addr_sp+0x100, 1, 0x4000-0x100, rom_file))==-1)
			perror("fread()");
	}
}

static void
lcd_control(int new_val)
{
	addr_sp[0xff41] = (new_val&0xf8) | (addr_sp[0xff41]&7);
}

static void
lcd_gen(int new_val)
{

}

static void
lcd_sptr(int new_val)
{
	;
}

static void
lcd_bg_disp(int new_val)
{
}

static void
lcd_bg_wt_dat_sel(int new_val)
{
	;
}

static void
lcd_init(int new_val)
{
	Uint8 i, j;
	static int lcd_enable=0;
	long *ptr_vh;
	addr_sp[0xff40] = new_val;
	
	if (lcd_enable==0 && (new_val&0x80))
	{
		lcd_enable=1;
	//if (addr_sp[0xff44] == addr_sp[0xff45])
	//	addr_sp[0xff41]|=4;
		/* This 'h-blk' is 80 cycles */
		gb_hblank_clks[0]=80;
		addr_sp[0xff41] = (addr_sp[0xff41]&0xfc);
		/* Start with mode 0 */
		addr_sp[0xff44] = 0;
	}
	/* 
	 * LCD is being turned off; reinitialize all values.
	 * LCD can only be turned off during VBLANK.
	 */
	else if (lcd_enable==1 && (!(new_val&0x80)))
	{
		lcd_enable=0;
		addr_sp[0xff41] &= 0xfc;
		addr_sp[0xff44] = 0x00;
		//ptr_vh = &gb_vbln_clks;
		//ptr_vh[0] = lcd_vbln_hbln_ctrl = ptr_vh[1];
		//SDL_FillRect(back, NULL, pal_grey[0]);
		//write_tmp();
		//for (i=0; i<170; i++)
		//	for (j=0; j<170; j++)
		//		back_col[i][j]=0;
	}
}

void (*lcdc_fptrs[8])(int) = { lcd_bg_disp, lcd_gen, lcd_gen, lcd_gen, lcd_bg_wt_dat_sel, lcd_gen, lcd_gen, lcd_init };

static void
lcd_ctrl(int lcdc_new)
{
	int i=1, m=0, cmp_val;
	static int lcdc_last=0;

	/* Iterate through bits */
	do {
		if ((lcdc_last & i) != (lcdc_new & i))
			lcdc_fptrs[m](lcdc_new&i);
		m++;
	} while ((i<<=1) < 0x100);

	lcdc_last=lcdc_new;
}

void
io_ctrl_wr(Uint8 io_off, Uint8 io_new)
{
	Uint8 *ptr_reg;

	switch (io_off) {
		case 0x00:
			joy_update(io_new);
			if (gboy_mode == SGB)
				write_sgb_packet(io_new);
			return;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			div_reset();
			return;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			tac_update(io_new);
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0a:
			break;
		case 0x0b:
			break;
		case 0x0c:
			break;
		case 0x0d:
			break;
		case 0x0e:
			break;
		case 0x0f:
			break;
		case 0x10:
			write_sound_reg(io_off, io_new);
			return;
		case 0x11:
			write_sound_reg(io_off, io_new);
			return;
		case 0x12:
			write_sound_reg(io_off, io_new);
			return;
		case 0x13:
			write_sound_reg(io_off, io_new);
			return;
		case 0x14:
			write_sound_reg(io_off, io_new);
			return;
		case 0x15:
			write_sound_reg(io_off, io_new);
			return;
		case 0x16:
			write_sound_reg(io_off, io_new);
			return;
		case 0x17:
			write_sound_reg(io_off, io_new);
			return;
		case 0x18:
			write_sound_reg(io_off, io_new);
			return;
		case 0x19:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1a:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1b:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1c:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1d:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1e:
			write_sound_reg(io_off, io_new);
			return;
		case 0x1f:
			write_sound_reg(io_off, io_new);
			return;
		case 0x20:
			write_sound_reg(io_off, io_new);
			return;
		case 0x21:
			write_sound_reg(io_off, io_new);
			return;
		case 0x22:
			write_sound_reg(io_off, io_new);
			return;
		case 0x23:
			write_sound_reg(io_off, io_new);
			return;
		case 0x24:
			write_sound_reg(io_off, io_new);
			return;
		case 0x25:
			write_sound_reg(io_off, io_new);
			return;
		case 0x26:
			write_sound_reg(io_off, io_new);
			return;
		case 0x27:
			write_sound_reg(io_off, io_new);
			return;
		case 0x28:
			write_sound_reg(io_off, io_new);
			return;
		case 0x29:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2a:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2b:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2c:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2d:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2e:
			write_sound_reg(io_off, io_new);
			return;
		case 0x2f:
			write_sound_reg(io_off, io_new);
			return;
		case 0x30:
			write_sound_reg(io_off, io_new);
			return;
		case 0x31:
			write_sound_reg(io_off, io_new);
			return;
		case 0x32:
			write_sound_reg(io_off, io_new);
			return;
		case 0x33:
			write_sound_reg(io_off, io_new);
			return;
		case 0x34:
			write_sound_reg(io_off, io_new);
			return;
		case 0x35:
			write_sound_reg(io_off, io_new);
			return;
		case 0x36:
			write_sound_reg(io_off, io_new);
			return;
		case 0x37:
			write_sound_reg(io_off, io_new);
			return;
		case 0x38:
			write_sound_reg(io_off, io_new);
			return;
		case 0x39:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3a:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3b:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3c:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3d:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3e:
			write_sound_reg(io_off, io_new);
			return;
		case 0x3f:
			write_sound_reg(io_off, io_new);
			return;
		case 0x40:
			lcd_ctrl(io_new);
			break;
		case 0x41:
			lcd_control(io_new);
			return;
		case 0x42:
			break;
		case 0x43:
			break;
		case 0x44:
			return; // read-only
		case 0x45:
			write_ly_reg(io_new);
			return;
		case 0x46:
			do_oam_dma((unsigned int)io_new);
			break;
		case 0x47:
			break;
		case 0x48:
			break;
		case 0x49:
			break;
		case 0x4a:
			break;
		case 0x4b:
			break;
		case 0x4c:
			break;
		case 0x4d:
			if (gboy_mode==1) // CGB ONLY
				cgb_speed_switch(io_new);
			return;
		case 0x4e:
			break;
		case 0x4f:
			if (gboy_mode==1)
				do_remap_vram(io_new);
			break;
		case 0x50:
			disable_boot();
			break;
		case 0x51:
			if (gboy_mode==1) // CGB ONLY
				write_hdma1(io_new);
			return;
		case 0x52:
			if (gboy_mode==1) // CGB ONLY
				write_hdma2(io_new);
			return;
		case 0x53:
			if (gboy_mode==1) // CGB ONLY
				write_hdma3(io_new);
			return;
		case 0x54:
			if (gboy_mode==1) // CGB ONLY
				write_hdma4(io_new);
			return;
		case 0x55:
			if (gboy_mode==1) // CGB ONLY
				write_vram_dma(io_new);
			return;
		case 0x56:
			break;
		case 0x57:
			break;
		case 0x58:
			break;
		case 0x59:
			break;
		case 0x5a:
			break;
		case 0x5b:
			break;
		case 0x5c:
			break;
		case 0x5d:
			break;
		case 0x5e:
			break;
		case 0x5f:
			break;
		case 0x60:
			break;
		case 0x61:
			break;
		case 0x62:
			break;
		case 0x63:
			break;
		case 0x64:
			break;
		case 0x65:
			break;
		case 0x66:
			break;
		case 0x67:
			break;
		case 0x68:
			if (gboy_mode==1) // CGB ONLY
				write_back(io_new);
			return;
		case 0x69:
			if (gboy_mode==1) // CGB ONLY
				write_back_pal(io_new);
			return;
		case 0x6a:
			if (gboy_mode==1) // CGB ONLY
				write_spr(io_new);
			return;
		case 0x6b:
			if (gboy_mode==1) // CGB ONLY
				write_spr_pal(io_new);
			return;
		case 0x6c:
			break;
		case 0x6d:
			break;
		case 0x6e:
			break;
		case 0x6f:
			break;
		case 0x70:
			if (gboy_mode==1) // CGB ONLY
				do_remap_wram(io_new);
			break;
		case 0x71:
			break;
		case 0x72:
			break;
		case 0x73:
			break;
		case 0x74:
			break;
		case 0x75:
			break;
		case 0x76:
			break;
		case 0x77:
			break;
		case 0x78:
			break;
		case 0x79:
			break;
		case 0x7a:
			break;
		case 0x7b:
			break;
		case 0x7c:
			break;
		case 0x7d:
			break;
		case 0x7e:
			break;
	}
	ptr_reg = addr_sp+io_off+0xff00; // pointer to register
	*ptr_reg &= 0x0; // reset register
	*ptr_reg |= io_new; // write new register
}
