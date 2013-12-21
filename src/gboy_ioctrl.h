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
#ifdef USE_X86_64_ASM
extern long tac_counter;
extern long tac_reload;
extern long tac_on;
#else
#include "gboy_cpu.h"
extern struct cpu_state cpu_state;
#endif
/* Special assembly-exports declarations */
extern Uint32 gboy_mode; // Game Boy/Color Game Boy mode
extern long gb_hblank_clks[2];
extern long gb_vbln_clks[2];
extern long lcd_vbln_hbln_ctrl;
extern long key_bitmap;
extern long addr_sp_ptrs[16];
extern void write_sound_reg(Uint8, Uint8);
extern void do_sgb_packet(Uint8);
extern void do_spr_pal_wr(Uint8);
extern void do_back_pal_wr(Uint8);
extern void do_spr_pal(Uint8);
extern void do_back_pal(Uint8);
extern void do_vram_dma(Uint8);
