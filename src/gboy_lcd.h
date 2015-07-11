/* RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
 * Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
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
#ifdef USE_X86_64_ASM
extern long cpu_cur_mode;
extern long hbln_dma_src;
extern long hbln_dma_dst;
extern long hbln_dma;
extern long hdma_on;
#else
extern struct gboy_cpu gboy_cpu;
#include "gboy_cpu.h"
#endif
static Uint8 back_col[170][170];
struct spr_attr {
	Sint16 x;
	Sint16 y;
	Uint16 tile_num;
	Uint8 xoff;
	Uint8 yoff;
	Uint8 sizey;
	Uint8 xflip;
	Uint8 yflip;
	Uint8 page;
	Uint8 pal_col;
	Uint8 pal;
	Uint8 priority;
} spr_attr[40];

/* Global/Exported variables */
long nb_spr;
Uint8 sgb_pal_map[20][18];
Uint32 spr_pal_inc_indx;
Uint32 spr_pal_cur_indx;
Uint32 pal_inc_indx;
Uint32 pal_cur_indx;
Uint16 spr_pal[8][4];
Uint16 bg_pal[8][4];
long spr_extr_cycles[11] = { 0, 8, 20, 32, 44, 52, 64, 76, 88, 96, 108 };
long spr_cur_extr=0;
long lcd_vbln_hbln_ctrl=0;
long gb_clk_rate = 4194304;
long gb_line_clks = 459;
long gb_vbln_clks[2] = { 4590, 4590 };
long gb_oam_clks[2] = { 80, 80 };
long gb_hblank_clks[2] = { 200, 200 };
long gb_vram_clks[2] = { 176, 176 };

/* Imported/External variables. Defined in gboy_video.c */
extern Uint32 gboy_mode; // Game Boy/Color Game Boy mode
#ifdef VITA
extern Surface *back;
#else
extern SDL_Surface *back;
#endif
extern Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
extern Uint32 pal_grey[4]; // 32-bit palette for monochrome Game Boy
extern Uint32 pal_color[32*32*32]; // 32-bit palette for Color Game Boy
long dma_pend=0;
extern long addr_sp_ptrs[16];
