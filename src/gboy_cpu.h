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
/* Misc */
#define NULLZ 0

/* Offsets to registers in address space */
#define DIV_REG 0xff04 // DIV
#define TIMA_REG 0xff05 // TIMA
#define TMA_REG 0xff06 // TMA
#define TAC_REG 0xff07 // TAC
#define LCDC_REG 0xff40 // LCD Control
#define LCDS_REG 0xff41 // LCD Status
#define SCR_X 0xff43 // SCROLL X
#define LY_REG 0xff44 // LY
#define LYC_REG 0xff45 // LYC
#define SPD_REG 0xff4d // Speed
#define VRAM_DMA_SRC 0xff51 // VRAM DMA Source
#define VRAM_DMA_DST 0xff53 // VRAM DMA Destination
#define VRAM_DMA 0xff55 // VRAM DMA
#define IR_REG 0xff0f // Interrupt Request
#define IE_REG 0xffff // Interrupt Enable

/* Interrupt masks in LCD Status register */
#define LY_LYC_FLAG 0x4 // LY/LYC coincidence
#define H_BLN_INT 0x08 // HBLANK interrupt
#define V_BLN_INT 0x10 // VBLANK interrupt
#define OAM_INT 0x20 // OAM interrupt
#define LY_LYC_INT 0x40 // LY/LYC coincidence interrupt

/* Bit-control masks for set, reset and bit operations */
#define BIT_1 0x01
#define BIT_2 0x02
#define BIT_3 0x04
#define BIT_4 0x08
#define BIT_5 0x10
#define BIT_6 0x20
#define BIT_7 0x40
#define BIT_8 0x80

/* Flag (F) register masks */
#define F_ZERO 0x80 // zero flag
#define F_SUBTRACT 0x40 // add/subtract flag
#define F_HCARRY 0x20 // half carry flag
#define F_CARRY 0x10 // carry flag

/* Addressing modes */
#define REG 0x02 // operand is register
#define REG_IND 0x04 // operand is in memory pointed by register
#define IMM 0x08 // operand is immediate
#define IMM_IND 0x10 // operand is in memory pointed by immediate value

/* Addressing bytes or words */
#define BYTE ~0xff
#define WORD ~0xffff

#define DELAY 0x4
#define RD_XOR_WR 0x2
#define RD_WR 0x1

struct cpu_state {
	Uint8 *pc;
	Uint8 cpu_halt;
	Uint8 inst_is_cb;
	Uint8 just_enabled;
	Uint8 del_wr;
	Uint16 del_io;
	Uint8 *del_addr;
	Uint8 cur_tcks;
	Uint32 write_is_delayed;
	Uint8 div_ctrl;
	long tac_on;
	long tac_counter;
	long tac_reload;
	long hdma_on;
	long hbln_dma_dst;
	long hbln_dma_src;
	long hbln_dma;
	Uint32 cpu_cur_mode;
	Uint32 ime_flag;
} cpu_state;

extern long spr_extr_cycles[11];
extern long spr_cur_extr;
extern long lcd_vbln_hbln_ctrl;
extern long gb_clk_rate;
extern long gb_line_clks;
extern long gb_vbln_clks[2];
extern long gb_oam_clks[2];
extern long gb_hblank_clks[2];
extern long gb_vram_clks[2];
extern Uint32 skip_next_frame;
extern long ints_offs[5];
extern long chg_gam;
extern Uint32 gboy_mode; // Game Boy/Color Game Boy mode
extern long nb_spr;
extern int gbddb;
extern Uint32 fullscreen;
extern void mem_wr(Uint16, Uint8, Uint8 *);
extern Uint8 mem_rd(Uint16, Uint8 *);
extern void io_ctrl_wr(Uint8, Uint8);
extern long proc_evts();
extern void vid_frame_update();
extern int frame_skip();
extern void render_scanline(long);
extern void gddb_main(int, Uint8 *, Uint8 *);

/*
 * Instruction format:
 * opcode, dest addr, dest, src addr, src, unit
 */
struct z80_set {
	Uint8 format[8];
	char name[16];
	long length;
	void (*func)(struct z80_set *);
};

struct regs_sets {
	union regs {
		Uint8 UByte[2];
		Sint8 SByte[2];
		Uint16 UWord;
		Sint16 SWord;
	} regs[11];
} regs_sets;
