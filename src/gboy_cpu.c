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

Uint8 cpu_halt=0;
Uint8 inst_is_cb=0;
Uint8 just_enabled=0;
Uint8 del_wr=0;
Uint8 *del_addr;
Uint16 del_io=0;
Uint8 cur_tcks=0;
Uint32 write_is_delayed=0;
long nb_spr=0;
long chg_gam=0;
Uint8 div_ctrl=0;
long tac_on=0;
long tac_counter=0;
Uint32 skip_next_frame=0;
long tac_reload=0;
long hdma_on=0;
long hbln_dma_dst;
long hbln_dma_src;
long hbln_dma=0;
Uint32 spr_extr_cycles[11] = { 0, 8, 20, 32, 44, 52, 64, 76, 88, 96, 108 };
Uint32 spr_cur_extr=0;
Uint32 ime_flag=0;
Uint32 cpu_cur_mode=0;
Uint32 lcd_vbln_hbln_ctrl=0;
Uint32 gb_clk_rate = 4194304;
Uint32 gb_line_clks = 459;
long gb_vbln_clks[2] = { 4590, 4590 };
Sint32 gb_oam_clks[2] = { 80, 80 };
long gb_hblank_clks[2] = { 200, 200 };
Sint32 gb_vram_clks[2] = { 176, 176 };
static Uint8 *pc;
extern int fullscr;

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
	} regs[10];
} regs_sets;
long addr_sp_ptrs[16] = { 0 };


/* 
 * Group 1: Load
 */
/* 
 * ld HL, SP+dd
 */
void
op_ld_sp_imm_hl(struct z80_set *rec)
{
	Uint16 acc;
	Sint32 dword;
	Sint8 sum;

	acc = regs_sets.regs[SP].UWord;
	sum = *(pc+1);
	regs_sets.regs[HL].SWord = dword = regs_sets.regs[SP].SWord+sum;
	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ (regs_sets.regs[HL].SWord))&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	/* 
	 * XXX Why does this work?
	 */
	if (regs_sets.regs[HL].UWord < sum)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else if (regs_sets.regs[HL].UByte[0] < ((Uint8)sum))
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

//	if (sum >= 0) {
//		if (regs_sets.regs[HL].UWord < acc)
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//		else if (regs_sets.regs[HL].UByte[0] < ((Uint8)sum))
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//	}
//	else {
//		if (!(regs_sets.regs[HL].UWord < acc))
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//		else if (!(regs_sets.regs[HL].UByte[0] < ((Uint8)sum)))
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//	}

	pc+=2;
	regs_sets.regs[PC].UWord+=2;

}
	

/* 
 * ld reg, reg
 *
 */
void
op_ld_reg_reg(struct z80_set *rec)
{
	regs_sets.regs[PC].UWord++;
	pc++;

	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1]= regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
}

/* 
 * ld (HL), imm
 *
 */
void
op_ld_imm_mem(struct z80_set *rec)
{
	Uint8 imm;

	regs_sets.regs[PC].UWord += 2;
	imm = pc[1];
	pc += 2;

	if ((regs_sets.regs[HL].UWord < 0xff80) && (regs_sets.regs[HL].UWord >= 0xff00))
		io_ctrl(regs_sets.regs[HL].UWord&0xff, imm);

	else if (regs_sets.regs[HL].UWord <= 0x7fff)
		((gb_mbc).mbc_funcs[(regs_sets.regs[HL].UWord>>12)])((int)imm);
	else {
		Uint8 *ptr_tmp;
		ptr_tmp = (Uint8 *)addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord;
		*ptr_tmp = imm;
	}
}

/* 
 * ld SP, HL
 */
void
op_ld_hl_sp(struct z80_set *rec)
{
	Uint16 *ptr_sp;

	regs_sets.regs[PC].UWord++;
	pc++;
	regs_sets.regs[SP].UWord = regs_sets.regs[HL].UWord;
}

/* 
 * ld SP, imm
 */
void
op_ld_imm_sp(struct z80_set *rec)
{
	Uint16 *ptr_imm;

	regs_sets.regs[PC].UWord += 3;
	
	ptr_imm = (Uint16 *)(pc+1);
	regs_sets.regs[SP].UWord = *ptr_imm;
	pc += 3;
}

/* 
 * ld reg, imm
 */
void
op_ld_imm_reg(struct z80_set *rec)
{
	switch (rec->length) {
		case BYTE:
				regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] = pc[1];
				pc+=2;
				regs_sets.regs[PC].UWord += 2;
				break;
		case WORD:
				regs_sets.regs[rec->format[2]].UWord = *((Uint16 *)(pc+1));
				pc+=3;
				regs_sets.regs[PC].UWord += 3;
				break;
	}
}

/*
 * ld (imm), reg
 */
void
op_ld_reg_imm(struct z80_set *rec)
{
	Uint16 addr;
	Uint8 *ptr_tmp;
	Uint16 *ptr_tmp_wr;

	switch (rec->length) {
		case BYTE:
				addr = *((Uint16 *)(pc+1));
				if (addr >= 0xff00 && addr < 0xff80)
					io_ctrl((Uint8)addr, regs_sets.regs[AF].UByte[A]);
				else if (addr <= 0x7fff)
					(gb_mbc.mbc_funcs[(addr>>12)])(regs_sets.regs[AF].UByte[A]);
				else {
					ptr_tmp = (Uint8 *)(addr_sp_ptrs[addr>>12]+addr);
					*ptr_tmp = regs_sets.regs[AF].UByte[A];
				}
				break;
		case WORD:
				addr = *((Uint16 *)(pc+1));
				if (addr >= 0xff00 && addr < 0xff80)
					io_ctrl((Uint8)addr, regs_sets.regs[SP].UByte[0]);
				else if (addr <= 0x7fff)
					(gb_mbc.mbc_funcs[(addr>>12)])(regs_sets.regs[SP].UWord);
				else {
					ptr_tmp_wr = (Uint16 *)(addr_sp_ptrs[addr>>12]+addr);
					*ptr_tmp_wr = regs_sets.regs[SP].UWord;
				}
				break;
	}

	pc+=3;
	regs_sets.regs[PC].UWord += 3;
}

/* 
 * ld mem, reg
 */
void
op_ld_reg_mem(struct z80_set *rec)
{

	Uint16 addr = (regs_sets.regs[rec->format[2]].UWord>>12);

	if (regs_sets.regs[rec->format[2]].UWord >= 0xff00 && regs_sets.regs[rec->format[2]].UWord < 0xff80)
		io_ctrl(regs_sets.regs[rec->format[2]].UWord&0xff, regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1]);

	else if (regs_sets.regs[rec->format[2]].UWord < 0x7fff)
		(gb_mbc.mbc_funcs[(regs_sets.regs[rec->format[2]].UWord>>12)])(regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1]);
					
	else {
		Uint8 *dest;
		dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[rec->format[2]].UWord>>12]+regs_sets.regs[rec->format[2]].UWord);
		*dest = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	}

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ld reg, mem
 */
void
op_ld_mem_reg(struct z80_set *rec)
{

	Uint8 *dest;

	dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[rec->format[4]].UWord>>12]+regs_sets.regs[rec->format[4]].UWord);
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] = *dest;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ldd (HL), A
 */
void
op_ldd_reg_mem(struct z80_set *rec)
{

	if (regs_sets.regs[HL].UWord >= 0xff00 && regs_sets.regs[HL].UWord < 0xff80)
		io_ctrl(regs_sets.regs[HL].UWord&0xff, regs_sets.regs[AF].UByte[A]);

	else if (regs_sets.regs[HL].UWord <= 0x7fff)
		(gb_mbc.mbc_funcs[regs_sets.regs[HL].UWord>>12])(regs_sets.regs[AF].UByte[A]);
	else {
		Uint8 *dest;
		dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
		*dest = regs_sets.regs[AF].UByte[A];
	}

	regs_sets.regs[HL].UWord--;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ldd A, (HL)
 */
void
op_ldd_mem_reg(struct z80_set *rec)
{
	Uint8 *dest;

	dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] = *dest;

	regs_sets.regs[HL].UWord--;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ldi (HL), A
 */
void
op_ldi_reg_mem(struct z80_set *rec)
{
	if (regs_sets.regs[HL].UWord >= 0xff00 && regs_sets.regs[HL].UWord < 0xff80)
		io_ctrl(regs_sets.regs[HL].UWord&0xff, regs_sets.regs[AF].UByte[A]);

	else if (regs_sets.regs[HL].UWord <= 0x7fff)
		(gb_mbc.mbc_funcs[regs_sets.regs[HL].UWord>>12])(regs_sets.regs[AF].UByte[A]);
	else {
		Uint8 *dest;
		dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
		*dest = regs_sets.regs[AF].UByte[A];
	}

	regs_sets.regs[HL].UWord++;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ldi A, (HL)
 */
void
op_ldi_mem_reg(struct z80_set *rec)
{
	Uint8 *dest;

	dest = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] = *dest;

	regs_sets.regs[HL].UWord++;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * ld A, (nn)
 */
void
op_ld_imm_acc(struct z80_set *rec)
{
	Uint8 *dest;
	Uint16 addr;

	addr = *((Uint16 *)(pc+1));

	dest = (Uint8 *)(addr_sp_ptrs[addr>>12]+addr);
	regs_sets.regs[AF].UByte[A] = *dest;

	pc+=3;
	regs_sets.regs[PC].UWord+=3;
}

/* 
 * ld A, (0xff00+C)
 */
void
op_ld_io_reg_reg(struct z80_set *rec)
{
	Uint8 *dest;
	Uint16 addr;

	regs_sets.regs[AF].UByte[A]= addr_sp[0xff00+(regs_sets.regs[BC].UByte[0])];

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ld A, (0xff00+imm)
 */
void
op_ld_io_reg_imm(struct z80_set *rec)
{
	Uint8 *dest;
	Uint16 addr;

	regs_sets.regs[AF].UByte[A]= addr_sp[(*(pc+1))+0xff00];

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * ld (0xff00+C), A
 */
void
op_ld_reg_io_reg(struct z80_set *rec)
{

	io_ctrl(regs_sets.regs[BC].UByte[0], regs_sets.regs[AF].UByte[A]);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * ld (0xff00+imm), A
 */
void
op_ld_reg_io_imm(struct z80_set *rec)
{
	io_ctrl(*(pc+1), regs_sets.regs[AF].UByte[A]);

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/*
 * pop AF
 */
void
op_pop_af(struct z80_set *rec)
{
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	regs_sets.regs[AF].UWord = *dest;
	regs_sets.regs[AF].UWord &= (0xff00|F_HCARRY|F_CARRY|F_ZERO|F_SUBTRACT);
	regs_sets.regs[SP].UWord += 2;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * pop BC, pop DE, pop HL
 */
void
op_pop(struct z80_set *rec)
{
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	regs_sets.regs[rec->format[2]].UWord = *dest;
	regs_sets.regs[SP].UWord += 2;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * push BC, push DE, push AF, push HL
 *
 */
void
op_push(struct z80_set *rec)
{
	Uint16 *dest;

	regs_sets.regs[SP].UWord -= 2;
	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = regs_sets.regs[rec->format[2]].UWord;

	pc++;
	regs_sets.regs[PC].UWord++;
}


/* 
 * Group 2: Arithmetic/Logic
 */
/* 
 * add A, reg8
 *
 */
void
op_add(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;

	acc = regs_sets.regs[AF].UByte[A];
	sum = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	regs_sets.regs[AF].UByte[A] += sum;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] < acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;


	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * add HL, reg16
 *
 */
void
op_add_wr(struct z80_set *rec)
{
	Uint16 acc;
	Uint16 sum;

	acc = regs_sets.regs[HL].UWord;
	sum = regs_sets.regs[rec->format[4]].UWord;
	regs_sets.regs[HL].UWord += sum;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY|F_CARRY);

	if ((acc ^ sum ^ regs_sets.regs[HL].UWord)&0x1000)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[HL].UWord < acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;


	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * add A, (HL)
 *
 */
void
op_add_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;

	acc = regs_sets.regs[AF].UByte[A];
	sum = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] += sum;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] < acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;


	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * add A, imm8
 *
 */
void
op_add_imm(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;

	acc = regs_sets.regs[AF].UByte[A];
	sum = *(pc+1);
	regs_sets.regs[AF].UByte[A] += sum;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] < acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * add SP, reg8
 */
void
op_add_sp_sign(struct z80_set *rec)
{
	Uint16 acc;
	Sint8 sum;

	acc = regs_sets.regs[SP].UWord;
	sum = *(pc+1);
	regs_sets.regs[SP].SWord += sum;

	regs_sets.regs[AF].UByte[F] = 0;


	if ((acc ^ sum ^ regs_sets.regs[SP].UWord)&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	/* 
	 * XXX Why does this work?
	 */
	if (regs_sets.regs[SP].UWord < sum)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else if (regs_sets.regs[SP].UByte[0] < ((Uint8)sum))
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

//	if (sum >= 0) {
//		if (regs_sets.regs[SP].UWord < acc)
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//		else if (regs_sets.regs[SP].UByte[0] < ((Uint8)acc))
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//	}
//	else {
//		if (regs_sets.regs[SP].UWord < sum)
//			regs_sets.regs[AF].UByte[F] |= F_CARRY;
//	}
//	
	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * adc reg8, reg8
 *
 */
void
op_adc(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sum = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	word += sum + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0);
	regs_sets.regs[AF].UByte[A] += sum + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0);

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

//if (regs_sets.regs[AF].UByte[A] < acc)
//	regs_sets.regs[AF].UByte[F] |= F_CARRY;
	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * adc A, (HL)
 *
 */
void
op_adc_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sum = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	word += sum + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0);
	regs_sets.regs[AF].UByte[A] += sum + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0);

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
//if (regs_sets.regs[AF].UByte[A] < acc)
//	regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * adc A, imm8
 *
 */
void
op_adc_imm(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sum;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sum = *(pc+1);
	regs_sets.regs[AF].UByte[A] += sum;
	word += sum;

	if (regs_sets.regs[AF].UByte[F] & F_CARRY)
		regs_sets.regs[AF].UByte[A] += 1, word += 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sum ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

//if (regs_sets.regs[AF].UByte[A] < acc)
//	regs_sets.regs[AF].UByte[F] |= F_CARRY;
	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * sub
 *
 */
void
op_sub(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;

	acc = regs_sets.regs[AF].UByte[A];
	sub = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	regs_sets.regs[AF].UByte[A] -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] > acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * sub A, (HL)
 *
 */
void
op_sub_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;

	acc = regs_sets.regs[AF].UByte[A];
	sub = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] > acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * sub A, imm8
 *
 */
void
op_sub_imm(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;

	acc = regs_sets.regs[AF].UByte[A];
	sub = *(pc+1);
	regs_sets.regs[AF].UByte[A] -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (regs_sets.regs[AF].UByte[A] > acc)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * sbc
 *
 */
void
op_sbc(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sub = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	word -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));
	regs_sets.regs[AF].UByte[A] -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

//if (regs_sets.regs[AF].UByte[A] > acc)
//	regs_sets.regs[AF].UByte[F] |= F_CARRY;
	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * sbc A, (HL)
 *
 */
void
op_sbc_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sub = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	word -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));
	regs_sets.regs[AF].UByte[A] -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
//if (regs_sets.regs[AF].UByte[A] > acc)
//	regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * sbc A, imm8
 *
 */
void
op_sbc_imm(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub;
	Uint16 word;

	acc = word = regs_sets.regs[AF].UByte[A];
	sub = *(pc+1);
	word -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));
	regs_sets.regs[AF].UByte[A] -= (sub + (regs_sets.regs[AF].UByte[F]&F_CARRY ? 1 : 0));

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ regs_sets.regs[AF].UByte[A])&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (word & 0xff00)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * xor accumulator, reg
 *
 */
void
op_xor_reg_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 xor;

	acc = regs_sets.regs[AF].UByte[A];
	xor = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	regs_sets.regs[AF].UByte[A] ^= xor;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;


	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * cp accumulator, mem
 *
 */
void
op_cp_mem_ind(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub, tmp;

	acc = tmp = regs_sets.regs[AF].UByte[A];
	sub = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	tmp -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ tmp)&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (acc < ((Uint8)(acc-sub)))
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (tmp == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * cp accumulator, mem
 *
 */
void
op_cp_imm_acc(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub, tmp;

	acc = tmp = regs_sets.regs[AF].UByte[A];
	sub = *(pc+1);
	tmp -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ tmp)&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (acc < ((Uint8)(acc-sub)))
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (tmp == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * cp A, reg
 *
 */
void
op_cp_reg(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 sub, tmp;

	acc = tmp = regs_sets.regs[AF].UByte[A];
	sub = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	tmp -= sub;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((acc ^ sub ^ tmp)&0x10)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	if (acc < ((Uint8)(acc-sub)))
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	if (tmp == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * and A, reg
 *
 */
void
op_and_reg_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 and;

	acc = regs_sets.regs[AF].UByte[A];
	and = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	regs_sets.regs[AF].UByte[A] &= and;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * and accumulator, (HL)
 *
 */
void
op_and_mem_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 and;

	acc = regs_sets.regs[AF].UByte[A];
	and = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] &= and;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * and accumulator, (imm)
 *
 */
void
op_and_imm_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 and;

	acc = regs_sets.regs[AF].UByte[A];
	and = *(pc+1);
	regs_sets.regs[AF].UByte[A] &= and;

	regs_sets.regs[AF].UByte[F] = 0;
	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * or accumulator, (imm)
 *
 */
void
op_or_imm_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 or;

	acc = regs_sets.regs[AF].UByte[A];
	or = *(pc+1);
	regs_sets.regs[AF].UByte[A] |= or;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * xor accumulator, (imm)
 *
 */
void
op_xor_imm_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 xor;

	acc = regs_sets.regs[AF].UByte[A];
	xor = *(pc+1);
	regs_sets.regs[AF].UByte[A] ^= xor;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc+=2;
	regs_sets.regs[PC].UWord+=2;
}

/* 
 * or accumulator, (HL)
 *
 */
void
op_or_mem_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 or;

	acc = regs_sets.regs[AF].UByte[A];
	or = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] |= or;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;


	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * xor accumulator, (HL)
 *
 */
void
op_xor_mem_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 xor;

	acc = regs_sets.regs[AF].UByte[A];
	xor = *(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[AF].UByte[A] ^= xor;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * or accumulator, reg
 *
 */
void
op_or_reg_accu(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 or;

	acc = regs_sets.regs[AF].UByte[A];
	or = regs_sets.regs[rec->format[4]&0xfe].UByte[rec->format[4]&1];
	regs_sets.regs[AF].UByte[A] |= or;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * dec (HL)
 *
 */
void
op_dec_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 dec;

	acc = regs_sets.regs[AF].UByte[A];
	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;
	dec = del_wr = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));

	regs_sets.regs[AF].UByte[F] &= ~(F_ZERO|F_HCARRY);

	dec--;
	if (dec == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if ((dec&0x0f) == 0x0f)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	del_wr--;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * inc (HL)
 *
 */
void
op_inc_mem(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 inc, tmp;

	acc = regs_sets.regs[AF].UByte[A];
	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;
	inc = del_wr = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_ZERO|F_HCARRY);

	inc++;
	if (inc == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if ((inc&0xf) == 0)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	del_wr++;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * dec register
 *
 */
void
op_dec_reg(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 dec;

	acc = regs_sets.regs[AF].UByte[A];
	dec = --regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];

	regs_sets.regs[AF].UByte[F] &= ~(F_ZERO|F_HCARRY);

	if (dec == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if ((dec&0x0f) == 0x0f)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	regs_sets.regs[AF].UByte[F] |= F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * inc register
 *
 */
void
op_inc_reg(struct z80_set *rec)
{
	Uint8 acc;
	Uint8 inc;

	acc = regs_sets.regs[AF].UByte[A];
	inc = ++regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_ZERO|F_HCARRY);

	if (inc == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if ((inc&0x0f) == 0)
		regs_sets.regs[AF].UByte[F] |= F_HCARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * dec r16
 *
 */
void
op_inc_reg_sp(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord++;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * dec r16
 *
 */
void
op_dec_reg_sp(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord--;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * dec r16
 *
 */
void
op_dec_reg_wr(struct z80_set *rec)
{
	regs_sets.regs[rec->format[2]].UWord--;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * inc r16
 *
 */
void
op_inc_reg_wr(struct z80_set *rec)
{
	regs_sets.regs[rec->format[2]].UWord++;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * Group 3: Rotate/Shift/Swap
 */
/* 
 * sla (HL)
 *
 */
void
op_shf_lf_arth_mem(struct z80_set *rec)
{
	Uint8 shf;
	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = shf = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr <<= 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if (((shf<<1)&0xff) == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * srl (HL)
 *
 */
void
op_shf_rgh_lg_mem(struct z80_set *rec)
{
	Uint8 shf;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = shf = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr >>= 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((shf>>1) == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * sra (HL)
 *
 */
void
op_shf_rgh_arth_mem(struct z80_set *rec)
{
	Sint8 shf;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	shf = (*(Sint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));

	regs_sets.regs[AF].UByte[F] = 0;

	if ((shf>>1) == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	del_wr = shf>>1;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * sra reg8
 *
 */
void
op_shf_rgh_arth(struct z80_set *rec)
{
	Sint8 shf;

	shf = regs_sets.regs[rec->format[2]&0xfe].SByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].SByte[rec->format[2]&1] >>= 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((shf>>1) == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * srl reg8
 *
 */
void
op_shf_rgh_lg(struct z80_set *rec)
{
	Uint8 shf;

	shf = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] >>= 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if ((shf>>1) == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * sla reg8
 *
 */
void
op_shf_lf_arth(struct z80_set *rec)
{
	Uint8 shf;

	shf = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] <<= 1;

	regs_sets.regs[AF].UByte[F] = 0;

	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	if (shf&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rr reg8
 *
 */
void
op_rot_rgh_reg(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] >>= 1;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] |= 0x80;
	else
		regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] &= ~0x80;

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rl reg8
 *
 */
void
op_rot_lf_reg(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] <<= 1;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] |= 0x01;
	else
		regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] &= ~0x01;

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rrc reg8
 *
 */
void
op_rot_rgh_c_reg(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] >>= 1;

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] |= 0x80;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] &= ~0x80;

	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rlc reg8
 *
 */
void
op_rot_lf_c_reg(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] <<= 1;

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] |= 0x01;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] &= ~0x01;

	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rlca
 *
 */
void
op_rot_lf_c_acc(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[AF].UByte[A];
	regs_sets.regs[AF].UByte[A] <<= 1;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY|F_ZERO);

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, regs_sets.regs[AF].UByte[A] |= 0x01;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, regs_sets.regs[AF].UByte[A] &= ~0x01;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rla
 *
 */
void
op_rot_lf_acc(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[AF].UByte[A];
	regs_sets.regs[AF].UByte[A] <<= 1;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY|F_ZERO);

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		regs_sets.regs[AF].UByte[A] |= 0x01;
	else
		regs_sets.regs[AF].UByte[A] &= ~0x01;

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rl (HL)
 *
 */
void
op_rot_lf_mem(struct z80_set *rec)
{
	Uint8 rot;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = rot = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr <<= 1;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		del_wr |= 0x01;
	else
		del_wr &= ~0x01;

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	if (del_wr == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rlc (HL)
 *
 */
void
op_rot_lf_c_mem(struct z80_set *rec)
{
	Uint8 rot;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = rot = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr <<= 1;

	if (rot&0x80)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, del_wr |= 0x01;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, del_wr &= ~0x01;

	if (del_wr == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;
	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rrc (HL)
 *
 */
void
op_rot_rgh_c_mem(struct z80_set *rec)
{
	Uint8 rot;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = rot = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr >>= 1;

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, del_wr |= 0x80;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, del_wr &= ~0x80;

	if (del_wr == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rr (HL)
 *
 */
void
op_rot_rgh_mem(struct z80_set *rec)
{
	Uint8 rot;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = rot = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	del_wr >>= 1;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		del_wr |= 0x80;
	else
		del_wr &= ~0x80;

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	if (del_wr == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rra
 *
 */
void
op_rot_rgh_acc(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[AF].UByte[A];
	regs_sets.regs[AF].UByte[A] >>= 1;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY|F_ZERO);

	if (regs_sets.regs[AF].UByte[F]&F_CARRY)
		regs_sets.regs[AF].UByte[A] |= 0x80;
	else
		regs_sets.regs[AF].UByte[A] &= ~0x80;

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * rrca
 *
 */
void
op_rot_rgh_c_acc(struct z80_set *rec)
{
	Uint8 rot;

	rot = regs_sets.regs[AF].UByte[A];
	regs_sets.regs[AF].UByte[A] >>= 1;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY|F_ZERO);

	if (rot&0x01)
		regs_sets.regs[AF].UByte[F] |= F_CARRY, regs_sets.regs[AF].UByte[A] |= 0x80;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY, regs_sets.regs[AF].UByte[A] &= ~0x80;

	pc++;
	regs_sets.regs[PC].UWord++;
}


/* 
 * swap r8
 *
 */
void
op_swp(struct z80_set *rec)
{
	Uint16 swp;

	swp = regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1];
	swp = ((swp<<4)|(swp>>4))&0xff;
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] = swp;

	regs_sets.regs[AF].UByte[F] = 0;

	if (swp == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * swap (HL)
 *
 */
void
op_swp_mem(struct z80_set *rec)
{
	Uint16 swp;

	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;

	del_wr = swp = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	swp = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord));
	swp = ((swp<<4)|(swp>>4))&0xff;
	del_wr = swp;

	regs_sets.regs[AF].UByte[F] = 0;

	if (swp == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/* 
 * Group 4: Single-bit operations
 */
/*
 * set BIT_NUM, (HL)
 *
 */
void
op_set_mem(struct z80_set *rec)
{
	del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	del_io = regs_sets.regs[HL].UWord;
	del_wr = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord)) | rec->format[3];
	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * set BIT_NUM, REG
 *
 */
void
op_set(struct z80_set *rec)
{
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] |= rec->format[3];
	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * rst BIT_NUM, REG
 *
 */
void
op_rst(struct z80_set *rec)
{
	regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1] &= ~rec->format[3];
	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * rst BIT_NUM, (HL)
 *
 */
void
op_rst_mem(struct z80_set *rec)
{
	/* XXX Ugly hack */
	if (regs_sets.regs[HL].UWord >= 0x8000) {
		del_addr = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
		del_io = regs_sets.regs[HL].UWord;
		del_wr = (*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord)) & ~rec->format[3];
	}
	else
		del_addr = NULLZ;
	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * bit BIT_NUM, REG
 *
 */
void
op_bit_mem(struct z80_set *rec)
{
	if ((*(Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord))&rec->format[3])
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_HCARRY;
	regs_sets.regs[AF].UByte[F] &= ~F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * bit BIT_NUM, REG
 *
 */
void
op_bit(struct z80_set *rec)
{
	if (regs_sets.regs[rec->format[2]&0xfe].UByte[rec->format[2]&1]&rec->format[3])
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] |= F_ZERO;

	regs_sets.regs[AF].UByte[F] |= F_HCARRY;
	regs_sets.regs[AF].UByte[F] &= ~F_SUBTRACT;

	pc++;
	regs_sets.regs[PC].UWord++;
}

void
op_daa(struct z80_set *rec)
{
	Uint16 acc, temp;

	pc++;
	regs_sets.regs[PC].UWord++;

	acc = regs_sets.regs[AF].UByte[A];
	temp = regs_sets.regs[AF].UByte[A];

	if (regs_sets.regs[AF].UByte[F]&F_SUBTRACT) {
		if (regs_sets.regs[AF].UByte[F]&F_HCARRY)
			acc -= 6, acc &= 0xff;
		if (regs_sets.regs[AF].UByte[F]&F_CARRY)
			acc -= 0x60;
	}
	else {
		if (!(regs_sets.regs[AF].UByte[F]&F_HCARRY)) {
			temp &= 0xf;
			if (temp > 9)
				acc += 6;
		}
		else
			acc += 6;
		if (!(regs_sets.regs[AF].UByte[F]&F_CARRY)) {
			if (acc > 0x9f)
				acc += 0x60;
		}
		else
			acc += 0x60;
	}

	regs_sets.regs[AF].UByte[F] &= ~F_HCARRY;
	regs_sets.regs[AF].UByte[A] = acc&0xff;
	acc &= 0x100;
	if (acc == 0x100)
		regs_sets.regs[AF].UByte[F] |= F_CARRY;
//else
//	regs_sets.regs[AF].UByte[F] &= ~F_CARRY;
	if (regs_sets.regs[AF].UByte[A] == 0)
		regs_sets.regs[AF].UByte[F] |= F_ZERO;
	else
		regs_sets.regs[AF].UByte[F] &= ~F_ZERO;
}

/* 
 * Group 5: CPU Control
 */
/*
 * nop
 *
 */
void
op_nop(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;
}

/*
 * stop
 *
 */
void
op_stop(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (addr_sp[SPD_REG]&0x01) {
		addr_sp[SPD_REG] = ~addr_sp[SPD_REG];
		addr_sp[SPD_REG] &= 0x80; 
		cpu_cur_mode = ~cpu_cur_mode;
		cpu_cur_mode &= 1;
	}
}

/* 
 * Group 6: Jump Instructions
 */
/*
 * callz
 *
 */
void
op_call_z(struct z80_set *rec)
{
	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (regs_sets.regs[AF].UByte[F]&F_ZERO) {
		cur_tcks += 12;
		regs_sets.regs[SP].UWord -= 2;
		Uint16 *dest;
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		*dest = regs_sets.regs[PC].UWord;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * callnz
 *
 */
void
op_call_nz(struct z80_set *rec)
{
	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (!(regs_sets.regs[AF].UByte[F]&F_ZERO)) {
		cur_tcks += 12;
		regs_sets.regs[SP].UWord -= 2;
		Uint16 *dest;
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		*dest = regs_sets.regs[PC].UWord;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * callnc
 *
 */
void
op_call_nc(struct z80_set *rec)
{
	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (!(regs_sets.regs[AF].UByte[F]&F_CARRY)) {
		cur_tcks += 12;
		regs_sets.regs[SP].UWord -= 2;
		Uint16 *dest;
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		*dest = regs_sets.regs[PC].UWord;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * callc
 *
 */
void
op_call_c(struct z80_set *rec)
{
	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY) {
		cur_tcks += 12;
		regs_sets.regs[SP].UWord -= 2;
		Uint16 *dest;
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		*dest = regs_sets.regs[PC].UWord;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * call
 *
 */
void
op_call(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = regs_sets.regs[PC].UWord+3;
	dest = (Uint16 *)pc;
	pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)+1))>>12]+(*(((Uint8 *)dest)+1)));
	regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)+1);
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_0(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp;
	regs_sets.regs[PC].UWord = 0;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_8h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+8;
	regs_sets.regs[PC].UWord = 8;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_10h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+16;
	regs_sets.regs[PC].UWord = 16;
}

/*
 * ret
 *
 */
void
op_ret(struct z80_set *rec)
{
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	regs_sets.regs[SP].UWord += 2;
	regs_sets.regs[PC].UWord = *dest;
	pc = (Uint8 *)addr_sp_ptrs[regs_sets.regs[PC].UWord>>12]+regs_sets.regs[PC].UWord;
}

void
op_reti(struct z80_set *rec)
{
	ime_flag = 1;
	op_ret(rec);
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_18h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+24;
	regs_sets.regs[PC].UWord = 24;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_20h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+32;
	regs_sets.regs[PC].UWord = 32;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_28h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+40;
	regs_sets.regs[PC].UWord = 40;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_30h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+48;
	regs_sets.regs[PC].UWord = 48;
}

/*
 * call
 *
 * finished: yes
 * tested: no
 */
void
op_reset_38h(struct z80_set *rec)
{
	regs_sets.regs[SP].UWord -= 2;
	
	Uint16 *dest;

	dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
	*dest = (regs_sets.regs[PC].UWord+1);
	dest = (Uint16 *)pc;
	pc = addr_sp+56;
	regs_sets.regs[PC].UWord = 56;
}

/*
 * ret z
 *
 */
void
op_ret_z(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (regs_sets.regs[AF].UByte[F]&F_ZERO) {

		Uint16 *dest;
		cur_tcks += 12;
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		regs_sets.regs[SP].UWord += 2;
		regs_sets.regs[PC].UWord = *dest;
		pc = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[PC].UWord>>12]+regs_sets.regs[PC].UWord);
	}
}

/*
 * ret nz
 *
 */
void
op_ret_nz(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (!(regs_sets.regs[AF].UByte[F]&F_ZERO)) {

		Uint16 *dest;
		cur_tcks += 12;
		
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		regs_sets.regs[SP].UWord += 2;
		regs_sets.regs[PC].UWord = *dest;
		pc = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[PC].UWord>>12]+regs_sets.regs[PC].UWord);
	}
}

/*
 * ret nc
 *
 */
void
op_ret_nc(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (!(regs_sets.regs[AF].UByte[F]&F_CARRY)) {

		Uint16 *dest;
		cur_tcks += 12;
		
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		regs_sets.regs[SP].UWord += 2;
		regs_sets.regs[PC].UWord = *dest;
		pc = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[PC].UWord>>12]+regs_sets.regs[PC].UWord);
	}
}

/*
 * ret c
 *
 */
void
op_ret_c(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY) {

		Uint16 *dest;
		cur_tcks += 12;
		
		dest = (Uint16 *)(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]+regs_sets.regs[SP].UWord);
		regs_sets.regs[SP].UWord += 2;
		regs_sets.regs[PC].UWord = *dest;
		pc = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[PC].UWord>>12]+regs_sets.regs[PC].UWord);
	}
}

/*
 * jmp a16
 *
 */
void
op_jmp(struct z80_set *rec)
{
	Uint16 *dest;

	dest = (Uint16 *)pc;
	pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)+1))>>12]+(*(((Uint8 *)dest)+1)));
	regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)+1);
}

/*
 * jnz a16
 *
 */
void
op_jmp_nz(struct z80_set *rec)
{
	Uint16 *dest;

	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (!(regs_sets.regs[AF].UByte[F]&F_ZERO)) {
		cur_tcks += 4;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * jz a16
 *
 */
void
op_jmp_z(struct z80_set *rec)
{
	Uint16 *dest;

	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (regs_sets.regs[AF].UByte[F]&F_ZERO) {
		cur_tcks += 4;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * jc a16
 *
 */
void
op_jmp_c(struct z80_set *rec)
{
	Uint16 *dest;

	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY) {
		cur_tcks += 4;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/*
 * jnc a16
 *
 */
void
op_jmp_nc(struct z80_set *rec)
{
	Uint16 *dest;

	pc+=3;
	regs_sets.regs[PC].UWord+=3;

	if (!(regs_sets.regs[AF].UByte[F]&F_CARRY)) {
		cur_tcks += 4;
		dest = (Uint16 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(*(((Uint8 *)dest)-2))>>12]+(*(((Uint8 *)dest)-2)));
		regs_sets.regs[PC].UWord = *(Uint16 *)(((Uint8 *)dest)-2);
	}
}

/* 
 * jr r8
 *
 */
void
op_jmp_rl(struct z80_set *rec)
{
	Sint8 *dest;
	pc+=2;
	regs_sets.regs[PC].UWord+=2;

	dest = (Sint8 *)pc;
	pc = (Uint8 *)(addr_sp_ptrs[(regs_sets.regs[PC].UWord+(*(dest-1)))>>12]+(regs_sets.regs[PC].UWord+(*(dest-1))));
	regs_sets.regs[PC].SWord += *(dest-1);

}

/* 
 * jrnz r8
 *
 */
void
op_jmp_rl_nz(struct z80_set *rec)
{
	Sint8 *dest;
	pc+=2;
	regs_sets.regs[PC].UWord+=2;

	if (!(regs_sets.regs[AF].UByte[F]&F_ZERO)) {
		cur_tcks += 4;
		dest = (Sint8 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(regs_sets.regs[PC].UWord+(*(dest-1)))>>12]+(regs_sets.regs[PC].UWord+(*(dest-1))));
		regs_sets.regs[PC].SWord += *(dest-1);
	}
}

/* 
 * jrz r8
 *
 */
void
op_jmp_rl_z(struct z80_set *rec)
{
	Sint8 *dest;
	pc+=2;
	regs_sets.regs[PC].UWord+=2;

	if (regs_sets.regs[AF].UByte[F]&F_ZERO) {
		cur_tcks += 4;
		dest = (Sint8 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(regs_sets.regs[PC].UWord+(*(dest-1)))>>12]+(regs_sets.regs[PC].UWord+(*(dest-1))));
		regs_sets.regs[PC].SWord += *(dest-1);
	}
}

/* 
 * jrnc r8
 *
 */
void
op_jmp_rl_nc(struct z80_set *rec)
{
	Sint8 *dest;
	pc+=2;
	regs_sets.regs[PC].UWord+=2;

	if (!(regs_sets.regs[AF].UByte[F]&F_CARRY)) {
		cur_tcks += 4;
		dest = (Sint8 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(regs_sets.regs[PC].UWord+(*(dest-1)))>>12]+(regs_sets.regs[PC].UWord+(*(dest-1))));
		regs_sets.regs[PC].SWord += *(dest-1);
	}
}

/* 
 * jrc r8
 *
 */
void
op_jmp_rl_c(struct z80_set *rec)
{
	Sint8 *dest;
	pc+=2;
	regs_sets.regs[PC].UWord+=2;

	if (regs_sets.regs[AF].UByte[F]&F_CARRY) {
		cur_tcks += 4;
		dest = (Sint8 *)pc;
		pc = (Uint8 *)(addr_sp_ptrs[(regs_sets.regs[PC].UWord+(*(dest-1)))>>12]+(regs_sets.regs[PC].UWord+(*(dest-1))));
		regs_sets.regs[PC].SWord += *(dest-1);
	}
}

/* 
 * jmp (HL)
 *
 */
void
op_jmp_ind(struct z80_set *rec)
{
	Uint16 *dest;

	dest = (Uint16 *)pc;
	pc = (Uint8 *)(addr_sp_ptrs[regs_sets.regs[HL].UWord>>12]+regs_sets.regs[HL].UWord);
	regs_sets.regs[PC].UWord = regs_sets.regs[HL].UWord;
}

/* 
 * di (disable interrupts)
 *
 */
void
op_dis_ints(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;
	ime_flag=0;
}

/* 
 * ei (enable interrupts)
 *
 */
void
op_ena_ints(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;
	ime_flag=1;
	just_enabled=1;
}

/* 
 * clear F_CARRY
 */
void
op_clr_car_flg(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	if (regs_sets.regs[AF].UByte[F] & F_CARRY)
		regs_sets.regs[AF].UByte[F] &= ~F_CARRY;
	else
		regs_sets.regs[AF].UByte[F] |= F_CARRY;

	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);
	
}

/* 
 * set F_CARRY
 */
void
op_set_car_flg(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;
	regs_sets.regs[AF].UByte[F] |= F_CARRY;
	regs_sets.regs[AF].UByte[F] &= ~(F_SUBTRACT|F_HCARRY);
}

/* 
 * flip accumulator
 */
void
op_cpl(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;
	regs_sets.regs[AF].UByte[F] |= (F_SUBTRACT|F_HCARRY);
	regs_sets.regs[AF].UByte[A] = ~regs_sets.regs[AF].UByte[A];
}


/*
 * Halt CPU
 */
void
op_halt(struct z80_set *rec)
{
	cpu_halt=1;
}

/*
 * INVALID INSTRUCTION!
 */
void
op_inval(struct z80_set *rec)
{
	while (1)
		;
}

/*
 * Opcode for escaping to 256 more opcodes.
 *
 */
void
op_escape(struct z80_set *rec)
{
	pc++;
	regs_sets.regs[PC].UWord++;

	inst_is_cb = 1;
}


struct z80_set z80_ldex[512] = 
{
	0x00, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	"nop\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_nop,
	
	0x01, REG, BC, IMM, NULLZ, NULLZ, 3, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_ld_imm_reg,
	
	0x02, REG_IND, BC, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ld (BC), A\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x03, REG, BC, NULLZ, NULLZ, NULLZ, 1, 8,
	 "inc BC\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_inc_reg_wr,
	
	0x04, REG, B, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x05, REG, B, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x06, REG, B, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x07, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "rlca\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_acc,
	
	0x08, IMM_IND, NULLZ, REG, SP, DELAY|RD_XOR_WR, 3, 20,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_ld_reg_imm,
	
	0x09, REG, HL, REG, BC, NULLZ, 1, 8,
	 "add HL, BC\0\0\0\0\0\0",
	WORD,
	op_add_wr,
	
	0x0a, REG, A, REG_IND, BC, DELAY|RD_XOR_WR, 1, 8,
	 "ld A, (BC)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x0b, REG, BC, NULLZ, NULLZ, NULLZ, 1, 8,
	 "dec BC\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_dec_reg_wr,
	
	0x0c, REG, C, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x0d, REG, C, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec C\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_dec_reg,
	
	0x0e, REG, C, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x0f, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "rrca\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_acc,
	
	0x10, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "stop\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_stop,
	
	0x11, REG, DE, IMM, NULLZ, NULLZ, 3, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_ld_imm_reg,
	
	0x12, REG_IND, DE, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ld (DE), A\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x13, REG, DE, NULLZ, NULLZ, NULLZ, 1, 8,
	 "inc DE\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_inc_reg_wr,
	
	0x14, REG, D, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x15, REG, D, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x16, REG, D, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x17, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "rla\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_acc,
	
	0x18, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 2, 12,
	 "jr\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_rl,
	
	0x19, REG, HL, REG, DE, NULLZ, 1, 8,
	 "add HL, DE\0\0\0\0\0\0",
	WORD,
	op_add_wr,
	
	0x1a, REG, A, REG_IND, DE, DELAY|RD_XOR_WR, 1, 8,
	 "ld A, (DE)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x1b, REG, DE, NULLZ, NULLZ, NULLZ, 1, 8,
	 "dec DE\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_dec_reg_wr,
	
	0x1c, REG, E, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x1d, REG, E, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x1e, REG, E, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x1f, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "rra\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_acc,
	
	0x20, NULLZ, F_ZERO, NULLZ, NULLZ, NULLZ, 2, 8,
	 "jrnz\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_rl_nz,
	
	0x21, REG, HL, IMM, NULLZ, NULLZ, 3, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_ld_imm_reg,
	
	0x22, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ldi (HL), A\0\0\0\0\0",
	WORD,
	op_ldi_reg_mem,
	
	0x23, REG, HL, NULLZ, NULLZ, NULLZ, 1, 8,
	 "inc HL\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_inc_reg_wr,
	
	0x24, REG, H, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x25, REG, H, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x26, REG, H, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x27, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "daa\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_daa,
	
	0x28, NULLZ, F_ZERO, NULLZ, NULLZ, NULLZ, 2, 8,
	 "jrz\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_rl_z,
	
	0x29, REG, HL, REG, HL, NULLZ, 1, 8,
	 "add HL, HL\0\0\0\0\0\0",
	WORD,
	op_add_wr,
	
	0x2a, REG, HL, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ldi A, (HL)\0\0\0\0\0",
	NULLZ,
	op_ldi_mem_reg,
	
	0x2b, REG, HL, NULLZ, NULLZ, NULLZ, 1, 8,
	 "dec HL\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_dec_reg_wr,
	
	0x2c, REG, L, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x2d, REG, L, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x2e, REG, L, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x2f, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "cpl\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_cpl,
	
	0x30, NULLZ, F_CARRY, NULLZ, NULLZ, NULLZ, 2, 8,
	 "jrnc\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_rl_nc,
	
	0x31, REG, SP, IMM, NULLZ, NULLZ, 3, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_ld_imm_sp,
	
	0x32, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ldd (HL), A\0\0\0\0\0",
	BYTE,
	op_ldd_reg_mem,
	
	0x33, REG, SP, NULLZ, NULLZ, NULLZ, 1, 8,
	 "inc SP\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg_sp,
	
	0x34, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 12,
	 "inc (HL)\0\0\0\0\0\0\0\0",
	WORD,
	op_inc_mem,
	
	0x35, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 12,
	 "dec (HL)\0\0\0\0\0\0\0\0",
	WORD,
	op_dec_mem,
	
	0x36, REG_IND, HL, IMM, NULLZ, DELAY|RD_XOR_WR, 2, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_mem,
	
	0x37, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "scf\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_set_car_flg,
	
	0x38, NULLZ, F_CARRY, NULLZ, NULLZ, NULLZ, 2, 8,
	 "jrc\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_rl_c,
	
	0x39, REG, HL, REG, SP, NULLZ, 1, 8,
	 "add HL, SP\0\0\0\0\0\0",
	WORD,
	op_add_wr,
	
	0x3a, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, NULLZ, 8,
	 "ldd A, (HL)\0\0\0\0\0",
	BYTE,
	op_ldd_mem_reg,
	
	0x3b, REG, SP, NULLZ, NULLZ, NULLZ, 1, 8,
	 "dec SP\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_dec_reg_sp,
	
	0x3c, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "inc A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_inc_reg,
	
	0x3d, REG, A, NULLZ, NULLZ, NULLZ, 1, 4,
	 "dec A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_dec_reg,
	
	0x3e, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_imm_reg,
	
	0x3f, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "ccf\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_clr_car_flg,
	
	0x40, REG, B, REG, B, NULLZ, 1, 4,
	 "ld B, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x41, REG, B, REG, C, NULLZ, 1, 4,
	 "ld B, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x42, REG, B, REG, D, NULLZ, 1, 4,
	 "ld B, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x43, REG, B, REG, E, NULLZ, 1, 4,
	 "ld B, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x44, REG, B, REG, H, NULLZ, 1, 4,
	 "ld B, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x45, REG, B, REG, L, NULLZ, 1, 4,
	 "ld B, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x46, REG, B, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld B, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x47, REG, B, REG, A, NULLZ, 1, 4,
	 "ld B, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x48, REG, C, REG, B, NULLZ, 1, 4,
	 "ld C, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x49, REG, C, REG, C, NULLZ, 1, 4,
	 "ld C, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x4a, REG, C, REG, D, NULLZ, 1, 4,
	 "ld C, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x4b, REG, C, REG, E, NULLZ, 1, 4,
	 "ld C, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x4c, REG, C, REG, H, NULLZ, 1, 4,
	 "ld C, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x4d, REG, C, REG, L, NULLZ, 1, 4,
	 "ld C, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x4e, REG, C, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld C, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x4f, REG, C, REG, A, NULLZ, 1, 4,
	 "ld C, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x50, REG, D, REG, B, NULLZ, 1, 4,
	 "ld D, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x51, REG, D, REG, C, NULLZ, 1, 4,
	 "ld D, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x52, REG, D, REG, D, NULLZ, 1, 4,
	 "ld D, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x53, REG, D, REG, E, NULLZ, 1, 4,
	 "ld D, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x54, REG, D, REG, H, NULLZ, 1, 4,
	 "ld D, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x55, REG, D, REG, L, NULLZ, 1, 4,
	 "ld D, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x56, REG, D, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld D, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x57, REG, D, REG, A, NULLZ, 1, 4,
	 "ld D, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x58, REG, E, REG, B, NULLZ, 1, 4,
	 "ld E, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x59, REG, E, REG, C, NULLZ, 1, 4,
	 "ld E, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x5a, REG, E, REG, D, NULLZ, 1, 4,
	 "ld E, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x5b, REG, E, REG, E, NULLZ, 1, 4,
	 "ld E, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x5c, REG, E, REG, H, NULLZ, 1, 4,
	 "ld E, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x5d, REG, E, REG, L, NULLZ, 1, 4,
	 "ld E, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x5e, REG, E, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld E, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x5f, REG, E, REG, A, NULLZ, 1, 4,
	 "ld E, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x60, REG, H, REG, B, NULLZ, 1, 4,
	 "ld H, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x61, REG, H, REG, C, NULLZ, 1, 4,
	 "ld H, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x62, REG, H, REG, D, NULLZ, 1, 4,
	 "ld H, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x63, REG, H, REG, E, NULLZ, 1, 4,
	 "ld H, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x64, REG, H, REG, H, NULLZ, 1, 4,
	 "ld H, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x65, REG, H, REG, L, NULLZ, 1, 4,
	 "ld H, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x66, REG, H, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld H, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x67, REG, H, REG, A, NULLZ, 1, 4,
	 "ld H, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x68, REG, L, REG, B, NULLZ, 1, 4,
	 "ld L, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x69, REG, L, REG, C, NULLZ, 1, 4,
	 "ld L, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x6a, REG, L, REG, D, NULLZ, 1, 4,
	 "ld L, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x6b, REG, L, REG, E, NULLZ, 1, 4,
	 "ld L, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x6c, REG, L, REG, H, NULLZ, 1, 4,
	 "ld L, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x6d, REG, L, REG, L, NULLZ, 1, 4,
	 "ld L, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x6e, REG, L, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld L, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x6f, REG, L, REG, A, NULLZ, 1, 4,
	 "ld L, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x70, REG_IND, HL, REG, B, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), B\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x71, REG_IND, HL, REG, C, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), C\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x72, REG_IND, HL, REG, D, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), D\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x73, REG_IND, HL, REG, E, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), E\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x74, REG_IND, HL, REG, H, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), H\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x75, REG_IND, HL, REG, L, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), L\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x76, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 8,
	 "halt\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_halt,
	
	0x77, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ld (HL), A\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_mem,
	
	0x78, REG, A, REG, B, NULLZ, 1, 4,
	 "ld A, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x79, REG, A, REG, C, NULLZ, 1, 4,
	 "ld A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x7a, REG, A, REG, D, NULLZ, 1, 4,
	 "ld A, D\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x7b, REG, A, REG, E, NULLZ, 1, 4,
	 "ld A, E\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x7c, REG, A, REG, H, NULLZ, 1, 4,
	 "ld A, H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x7d, REG, A, REG, L, NULLZ, 1, 4,
	 "ld A, L\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x7e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "ld A, (HL)\0\0\0\0\0\0",
	BYTE,
	op_ld_mem_reg,
	
	0x7f, REG, A, REG, A, NULLZ, 1, 4,
	 "ld A, A\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_reg,
	
	0x80, REG, A, REG, B, NULLZ, 1, 4,
	 "add A, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x81, REG, A, REG, C, NULLZ, 1, 4,
	 "add A, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x82, REG, A, REG, D, NULLZ, 1, 4,
	 "add A, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x83, REG, A, REG, E, NULLZ, 1, 4,
	 "add A, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x84, REG, A, REG, H, NULLZ, 1, 4,
	 "add A, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x85, REG, A, REG, L, NULLZ, 1, 4,
	 "add A, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x86, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "add A, (HL)\0\0\0\0\0",
	BYTE,
	op_add_mem,
	
	0x87, REG, A, REG, A, NULLZ, 1, 4,
	 "add A, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_add,
	
	0x88, REG, A, REG, B, NULLZ, 1, 4,
	 "adc A, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x89, REG, A, REG, C, NULLZ, 1, 4,
	 "adc A, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x8a, REG, A, REG, D, NULLZ, 1, 4,
	 "adc A, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x8b, REG, A, REG, E, NULLZ, 1, 4,
	 "adc A, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x8c, REG, A, REG, H, NULLZ, 1, 4,
	 "adc A, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x8d, REG, A, REG, L, NULLZ, 1, 4,
	 "adc A, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x8e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "adc A, (HL)\0\0\0\0\0",
	BYTE,
	op_adc_mem,
	
	0x8f, REG, A, REG, A, NULLZ, 1, 4,
	 "adc A, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc,
	
	0x90, REG, A, REG, B, NULLZ, 1, 4,
	 "sub A, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x91, REG, A, REG, C, NULLZ, 1, 4,
	 "sub A, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x92, REG, A, REG, D, NULLZ, 1, 4,
	 "sub A, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x93, REG, A, REG, E, NULLZ, 1, 4,
	 "sub A, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x94, REG, A, REG, H, NULLZ, 1, 4,
	 "sub A, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x95, REG, A, REG, L, NULLZ, 1, 4,
	 "sub A, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x96, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "sub A, (HL)\0\0\0\0\0",
	BYTE,
	op_sub_mem,
	
	0x97, REG, A, REG, A, NULLZ, 1, 4,
	 "sub A, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub,
	
	0x98, REG, A, REG, B, NULLZ, 1, 4,
	 "sbc A, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x99, REG, A, REG, C, NULLZ, 1, 4,
	 "sbc A, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x9a, REG, A, REG, D, NULLZ, 1, 4,
	 "sbc A, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x9b, REG, A, REG, E, NULLZ, 1, 4,
	 "sbc A, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x9c, REG, A, REG, H, NULLZ, 1, 4,
	 "sbc A, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x9d, REG, A, REG, L, NULLZ, 1, 4,
	 "sbc A, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0x9e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "sbc A, (HL)\0\0\0\0\0",
	BYTE,
	op_sbc_mem,
	
	0x9f, REG, A, REG, A, NULLZ, 1, 4,
	 "sbc A, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc,
	
	0xa0, REG, A, REG, B, NULLZ, 1, 4,
	 "and A, B\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa1, REG, A, REG, C, NULLZ, 1, 4,
	 "and A, C\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa2, REG, A, REG, D, NULLZ, 1, 4,
	 "and A, D\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa3, REG, A, REG, E, NULLZ, 1, 4,
	 "and A, E\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa4, REG, A, REG, H, NULLZ, 1, 4,
	 "and A, H\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa5, REG, A, REG, L, NULLZ, 1, 4,
	 "and A, L\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa6, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "and A, (HL)\0\0\0\0\0",
	NULLZ,
	op_and_mem_accu,
	
	0xa7, REG, A, REG, A, NULLZ, 1, 4,
	 "and A, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_reg_accu,
	
	0xa8, REG, A, REG, B, NULLZ, 1, 4,
	 "xor B, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xa9, REG, A, REG, C, NULLZ, 1, 4,
	 "xor C, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xaa, REG, A, REG, D, NULLZ, 1, 4,
	 "xor D, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xab, REG, A, REG, E, NULLZ, 1, 4,
	 "xor E, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xac, REG, A, REG, H, NULLZ, 1, 4,
	 "xor H, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xad, REG, A, REG, L, NULLZ, 1, 4,
	 "xor L, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xae, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "xor (HL), A\0\0\0\0\0",
	NULLZ,
	op_xor_mem_accu,
	
	0xaf, REG, A, REG, A, NULLZ, 1, 4,
	 "xor A, A\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_reg_accu,
	
	0xb0, REG, A, REG, B, NULLZ, 1, 4,
	 "or A, B\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb1, REG, A, REG, C, NULLZ, 1, 4,
	 "or A, C\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb2, REG, A, REG, D, NULLZ, 1, 4,
	 "or A, D\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb3, REG, A, REG, E, NULLZ, 1, 4,
	 "or A, E\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb4, REG, A, REG, H, NULLZ, 1, 4,
	 "or A, H\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb5, REG, A, REG, L, NULLZ, 1, 4,
	 "or A, L\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb6, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "or A, (HL)\0\0\0\0\0\0",
	NULLZ,
	op_or_mem_accu,
	
	0xb7, REG, A, REG, A, NULLZ, 1, 4,
	 "or A, A\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_or_reg_accu,
	
	0xb8, REG, A, REG, B, NULLZ, 1, 4,
	 "cp A, B\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xb9, REG, A, REG, C, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xba, REG, A, REG, D, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xbb, REG, A, REG, E, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xbc, REG, A, REG, H, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xbd, REG, A, REG, L, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xbe, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_mem_ind,
	
	0xbf, REG, A, REG, A, NULLZ, 1, 4,
	 "cp A, C\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_reg,
	
	0xc0, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 8,
	 "retnz\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ret_nz,
	
	0xc1, REG, BC, NULLZ, NULLZ, NULLZ, 1, 12,
	 "pop BC\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_pop,
	
	0xc2, NULLZ, F_ZERO, NULLZ, NULLZ, NULLZ, 3, 12,
	 "jnz\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_nz,
	
	0xc3, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 3, 16,
	 "jmp\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp,
	
	0xc4, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 3, 12,
	 "callnz\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_call_nz,
	
	0xc5, REG, BC, NULLZ, NULLZ, NULLZ, 1, 16,
	 "push BC\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_push,
	
	0xc6, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "add\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_add_imm,
	
	0xc7, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst \0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_reset_0,
	
	0xc8, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 8,
	 "retz\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ret_z,
	
	0xc9, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "ret\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ret,
	
	0xca, NULLZ, F_ZERO, NULLZ, NULLZ, NULLZ, 3, 12,
	 "jz\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_z,
	
	0xcb, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 0,
	 "escape\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_escape,
	
	0xcc, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 3, 12,
	 "callz\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_call_z,
	
	0xcd, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 3, 24,
	 "call\0\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_call,
	
	0xce, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "adc\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_adc_imm,
	
	0xcf, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 8H\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_8h,
	
	0xd0, C, NULLZ, NULLZ, NULLZ, NULLZ, 1, 8,
	 "retnc\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ret_nc,
	
	0xd1, REG, DE, NULLZ, NULLZ, NULLZ, 1, 12,
	 "pop DE\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_pop,
	
	0xd2, NULLZ, F_CARRY, NULLZ, NULLZ, NULLZ, 3, 12,
	 "jnc\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_nc,
	
	0xd3, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xd4, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 3, 12,
	 "callnc\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_call_nc,
	
	0xd5, REG, DE, NULLZ, NULLZ, NULLZ, 1, 16,
	 "push DE\0\0\0\0\0\0\0\0\0",
	WORD,
	op_push,
	
	0xd6, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "sub\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_sub_imm,
	
	0xd7, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 10H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_10h,
	
	0xd8, C, NULLZ, NULLZ, NULLZ, NULLZ, 1, 8,
	 "retc\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ret_c,
	
	0xd9, REG, B, REG, NULLZ, NULLZ, 1, 16,
	 "reti\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_reti,
	
	0xda, NULLZ, F_CARRY, NULLZ, NULLZ, NULLZ, 3, 12,
	 "jc\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_jmp_c,
	
	0xdb, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xdc, C, NULLZ, NULLZ, NULLZ, NULLZ, 3, 12,
	 "callc\0\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_call_c,
	
	0xdd, REG, B, REG, NULLZ, NULLZ, 1, 8,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xde, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "sbc\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_sbc_imm,
	
	0xdf, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 18H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_18h,
	
	0xe0, 1, NULLZ, REG, A, DELAY|RD_XOR_WR, 2, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_io_imm,
	
	0xe1, REG, HL, NULLZ, NULLZ, NULLZ, 1, 12,
	 "pop HL\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_pop,
	
	0xe2, 0x18, B, REG, A, DELAY|RD_XOR_WR, 1, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_io_reg,
	
	0xe3, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xe4, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xe5, REG, HL, NULLZ, NULLZ, NULLZ, 1, 16,
	 "push HL\0\0\0\0\0\0\0\0\0",
	WORD,
	op_push,
	
	0xe6, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "and A,\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_and_imm_accu,
	
	0xe7, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 20H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_20h,
	
	0xe8, REG, B, REG, NULLZ, NULLZ, 1, 16,
	 "add SP,\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_add_sp_sign,
	
	0xe9, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_XOR_WR, 1, 4,
	 "jmp (HL)\0\0\0\0\0\0\0\0",
	WORD,
	op_jmp_ind,
	
	0xea, IMM_IND, NULLZ, REG, A, DELAY|RD_XOR_WR, 3, 16,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_reg_imm,
	
	0xeb, REG, DE, REG, HL, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_inval,
	
	0xec, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xed, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xee, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "xor A,\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_xor_imm_accu,
	
	0xef, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 28H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_28h,
	
	0xf0, REG, A, 1, NULLZ, DELAY|RD_XOR_WR, 2, 12,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_ld_io_reg_imm,
	
	0xf1, REG, AF, NULLZ, NULLZ, NULLZ, 1, 12,
	 "pop AF\0\0\0\0\0\0\0\0\0\0",
	WORD,
	op_pop_af,
	
	0xf2, REG, A, 0x18, NULLZ, DELAY|RD_XOR_WR, 1, 8,
	 "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_ld_io_reg_reg,
	
	0xf3, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "di\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_dis_ints,
	
	0xf4, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "INVALD\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_inval,
	
	0xf5, REG, AF, NULLZ, NULLZ, NULLZ, 1, 16,
	 "push AF\0\0\0\0\0\0\0\0\0",
	WORD,
	op_push,
	
	0xf6, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "or A,\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_or_imm_accu,
	
	0xf7, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	 "rst 30H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_30h,
	
	0xf8, REG, B, REG, NULLZ, NULLZ, 1, 12,
	 "ld sp,\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_ld_sp_imm_hl,
	
	0xf9, REG, SP, REG, HL, NULLZ, 1, 8,
	 "ld SP, HL\0\0\0\0\0\0\0",
	WORD,
	op_ld_hl_sp,
	
	0xfa, REG, B, REG, NULLZ, DELAY|RD_XOR_WR, 3, 16,
	 "ld A, (nn)\0\0\0\0\0\0",
	NULLZ,
	op_ld_imm_acc,
	
	0xfb, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 4,
	 "ei\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	op_ena_ints,
	
	0xfc, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "ld sp,\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	0x00000000,
	
	0xfd, REG, B, REG, NULLZ, NULLZ, 1, 4,
	 "ld sp,\0\0\0\0\0\0\0\0\0\0",
	NULLZ,
	0x00000000,
	
	0xfe, REG, A, IMM, NULLZ, NULLZ, 2, 8,
	 "cp A,\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_cp_imm_acc,
	
	0xff, NULLZ, NULLZ, NULLZ, NULLZ, NULLZ, 1, 16,
	"rst 38H\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_reset_38h,

	0x00, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x01, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x02, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x03, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x04, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x05, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x06, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "rlc (HL)\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_mem,
	
	0x07, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rlc A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_c_reg,
	
	0x08, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x09, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x0a, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x0b, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x0c, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x0d, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x0e, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "rrc (HL)\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_mem,
	
	0x0f, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rrc A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_c_reg,
	
	0x10, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl B\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x11, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl C\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x12, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl D\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x13, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl E\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x14, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl H\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x15, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl L\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x16, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "rl (HL)\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_mem,
	
	0x17, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rl A\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_lf_reg,
	
	0x18, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr B\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x19, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr C\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x1a, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr D\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x1b, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr E\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x1c, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr H\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x1d, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr L\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x1e, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "rr (HL)\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_mem,
	
	0x1f, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "rr A\0\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_rot_rgh_reg,
	
	0x20, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x21, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x22, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x23, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x24, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x25, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x26, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "sla (HL)\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth_mem,
	
	0x27, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sla A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_lf_arth,
	
	0x28, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x29, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x2a, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x2b, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x2c, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x2d, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x2e, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "sra (HL)\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth_mem,
	
	0x2f, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "sra A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_arth,
	
	0x30, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap B\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x31, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap C\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x32, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap D\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x33, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap E\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x34, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap H\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x35, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap L\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x36, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "swap (HL)\0\0\0\0\0\0\0",
	BYTE,
	op_swp_mem,
	
	0x37, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "swap A\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_swp,
	
	0x38, REG, B, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl B\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x39, REG, C, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl C\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x3a, REG, D, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl D\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x3b, REG, E, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl E\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x3c, REG, H, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl H\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x3d, REG, L, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl L\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x3e, REG_IND, HL, NULLZ, NULLZ, DELAY|RD_WR, 1, 16,
	 "srl (HL)\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg_mem,
	
	0x3f, REG, A, NULLZ, NULLZ, NULLZ, 1, 8,
	 "srl A\0\0\0\0\0\0\0\0\0\0\0",
	BYTE,
	op_shf_rgh_lg,
	
	0x40, REG, B, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x41, REG, C, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x42, REG, D, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x43, REG, E, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x44, REG, H, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x45, REG, L, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x46, REG_IND, HL, BIT_1, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 1, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x47, REG, A, BIT_1, NULLZ, NULLZ, 1, 8,
	 "bit 1, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x48, REG, B, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x49, REG, C, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x4a, REG, D, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x4b, REG, E, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x4c, REG, H, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x4d, REG, L, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x4e, REG_IND, HL, BIT_2, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 2, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x4f, REG, A, BIT_2, NULLZ, NULLZ, 1, 8,
	 "bit 2, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x50, REG, B, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x51, REG, C, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x52, REG, D, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x53, REG, E, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x54, REG, H, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x55, REG, L, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x56, REG_IND, HL, BIT_3, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 3, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x57, REG, A, BIT_3, NULLZ, NULLZ, 1, 8,
	 "bit 3, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x58, REG, B, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x59, REG, C, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x5a, REG, D, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x5b, REG, E, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x5c, REG, H, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x5d, REG, L, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x5e, REG_IND, HL, BIT_4, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 4, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x5f, REG, A, BIT_4, NULLZ, NULLZ, 1, 8,
	 "bit 4, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x60, REG, B, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x61, REG, C, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x62, REG, D, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x63, REG, E, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x64, REG, H, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x65, REG, L, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x66, REG_IND, HL, BIT_5, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 5, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x67, REG, A, BIT_5, NULLZ, NULLZ, 1, 8,
	 "bit 5, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x68, REG, B, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x69, REG, C, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x6a, REG, D, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x6b, REG, E, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x6c, REG, H, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x6d, REG, L, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x6e, REG_IND, HL, BIT_6, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 6, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x6f, REG, A, BIT_6, NULLZ, NULLZ, 1, 8,
	 "bit 6, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x70, REG, B, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x71, REG, C, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x72, REG, D, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x73, REG, E, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x74, REG, H, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x75, REG, L, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x76, REG_IND, HL, BIT_7, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 7, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x77, REG, A, BIT_7, NULLZ, NULLZ, 1, 8,
	 "bit 7, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x78, REG, B, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x79, REG, C, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, C\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x7a, REG, D, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, D\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x7b, REG, E, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, E\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x7c, REG, H, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, H\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x7d, REG, L, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, L\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x7e, REG_IND, HL, BIT_8, NULLZ, DELAY|RD_XOR_WR, 1, 12,
	 "bit 8, (HL)\0\0\0\0\0",
	BYTE,
	op_bit_mem,
	
	0x7f, REG, A, BIT_8, NULLZ, NULLZ, 1, 8,
	 "bit 8, A\0\0\0\0\0\0\0\0",
	BYTE,
	op_bit,
	
	0x80, REG, B, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x81, REG, C, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x82, REG, D, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x83, REG, E, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x84, REG, H, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x85, REG, L, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x86, REG_IND, HL, BIT_1, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0x87, REG, A, BIT_1, NULLZ, NULLZ, 1, 8,
	 "rst 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x88, REG, B, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x89, REG, C, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x8a, REG, D, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x8b, REG, E, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x8c, REG, H, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x8d, REG, L, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x8e, REG_IND, HL, BIT_2, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0x8f, REG, A, BIT_2, NULLZ, NULLZ, 1, 8,
	 "rst 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x90, REG, B, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x91, REG, C, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x92, REG, D, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x93, REG, E, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x94, REG, H, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x95, REG, L, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x96, REG_IND, HL, BIT_3, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0x97, REG, A, BIT_3, NULLZ, NULLZ, 1, 8,
	 "rst 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x98, REG, B, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x99, REG, C, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x9a, REG, D, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x9b, REG, E, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x9c, REG, H, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x9d, REG, L, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0x9e, REG_IND, HL, BIT_4, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0x9f, REG, A, BIT_4, NULLZ, NULLZ, 1, 8,
	 "rst 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa0, REG, B, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa1, REG, C, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa2, REG, D, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa3, REG, E, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa4, REG, H, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa5, REG, L, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa6, REG_IND, HL, BIT_5, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0xa7, REG, A, BIT_5, NULLZ, NULLZ, 1, 8,
	 "rst 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa8, REG, B, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xa9, REG, C, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xaa, REG, D, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xab, REG, E, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xac, REG, H, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xad, REG, L, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xae, REG_IND, HL, BIT_6, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0xaf, REG, A, BIT_6, NULLZ, NULLZ, 1, 8,
	 "rst 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb0, REG, B, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb1, REG, C, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb2, REG, D, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb3, REG, E, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb4, REG, H, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb5, REG, L, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb6, REG_IND, HL, BIT_7, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0xb7, REG, A, BIT_7, NULLZ, NULLZ, 1, 8,
	 "rst 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb8, REG, B, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xb9, REG, C, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xba, REG, D, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xbb, REG, E, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xbc, REG, H, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xbd, REG, L, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xbe, REG_IND, HL, BIT_8, NULLZ, DELAY|RD_WR, 1, 16,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst_mem,
	
	0xbf, REG, A, BIT_8, NULLZ, NULLZ, 1, 8,
	 "rst 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_rst,
	
	0xc0, REG, B, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc1, REG, C, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc2, REG, D, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc3, REG, E, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc4, REG, H, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc5, REG, L, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc6, REG_IND, HL, BIT_1, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xc7, REG, A, BIT_1, NULLZ, NULLZ, 1, 8,
	 "set 1, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc8, REG, B, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xc9, REG, C, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xca, REG, D, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xcb, REG, E, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xcc, REG, H, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xcd, REG, L, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xce, REG_IND, HL, BIT_2, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xcf, REG, A, BIT_2, NULLZ, NULLZ, 1, 8,
	 "set 2, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd0, REG, B, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd1, REG, C, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd2, REG, D, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd3, REG, E, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd4, REG, H, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd5, REG, L, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd6, REG_IND, HL, BIT_3, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xd7, REG, A, BIT_3, NULLZ, NULLZ, 1, 8,
	 "set 3, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd8, REG, B, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xd9, REG, C, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xda, REG, D, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xdb, REG, E, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xdc, REG, H, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xdd, REG, L, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xde, REG_IND, HL, BIT_4, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xdf, REG, A, BIT_4, NULLZ, NULLZ, 1, 8,
	 "set 4, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe0, REG, B, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe1, REG, C, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe2, REG, D, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe3, REG, E, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe4, REG, H, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe5, REG, L, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe6, REG_IND, HL, BIT_5, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xe7, REG, A, BIT_5, NULLZ, NULLZ, 1, 8,
	 "set 5, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe8, REG, B, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xe9, REG, C, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xea, REG, D, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xeb, REG, E, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xec, REG, H, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xed, REG, L, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xee, REG_IND, HL, BIT_6, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xef, REG, A, BIT_6, NULLZ, NULLZ, 1, 8,
	 "set 6, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf0, REG, B, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf1, REG, C, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf2, REG, D, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf3, REG, E, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf4, REG, H, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf5, REG, L, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf6, REG_IND, HL, BIT_7, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xf7, REG, A, BIT_7, NULLZ, NULLZ, 1, 8,
	 "set 7, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf8, REG, B, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xf9, REG, C, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xfa, REG, D, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xfb, REG, E, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xfc, REG, H, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xfd, REG, L, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set,
	
	0xfe, REG_IND, HL, BIT_8, NULLZ, DELAY|RD_WR, 1, 16,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set_mem,
	
	0xff, REG, A, BIT_8, NULLZ, NULLZ, 1, 8,
	 "set 8, B\0\0\0\0\0\0\0\0",
	BYTE,
	op_set
};

void
execute_precise(struct z80_set *rec)
{
	Sint8 ticks;
	Uint8 div_tmp;

	write_is_delayed = 0;
	ticks = (rec->format[7]>>2)-1;
	div_tmp = div_ctrl;
	
	while (1)
	{
		if (write_is_delayed&0x3)
		{
			if (write_is_delayed&0x1)
			{
				if (del_addr != NULLZ) {
					*del_addr = del_wr;
					if (del_io >= 0xff00 && del_io < 0xff80)
						io_ctrl(del_io&0xff, del_wr);
				}
				write_is_delayed = 2;
				goto last_run;
			}
			else
				goto end_execute_precise;
		}
		else
		{
			ticks--;
			if (ticks == 0)
			{
				if (rec->format[5]&RD_WR)
				{
					write_is_delayed |= rec->format[5];
					rec->func(rec);
				}
			}
			else if (ticks<0)
			{
				write_is_delayed |= rec->format[5];
				rec->func(rec);
			}
last_run:
			if ((div_ctrl&0xf) > ((div_ctrl+4)&0xf))
			{
				if (tac_on&0x1)
				{
					if (--tac_counter == 0)
					{
						tac_counter = tac_reload;
						if ((++addr_sp[TIMA_REG]) == 0)
						{
							addr_sp[IR_REG] |= 4;
							addr_sp[TIMA_REG] = addr_sp[TMA_REG];
						}
					}
				}
			}
			div_ctrl += 4;
			if (div_ctrl==0)
				addr_sp[DIV_REG]++;
		}
	}

end_execute_precise:
	return;
}
void
timer_divider_update()
{
	Sint8 ticks;
	Uint8 div_tmp;

	ticks = (Sint8)cur_tcks;
	div_tmp = div_ctrl;
	if (tac_on&1)
	{
		while (ticks > 0)
		{
			if ((div_ctrl&0xf) > ((div_ctrl+4)&0xf))
			{
				if (--tac_counter == 0)
				{
					tac_counter = tac_reload;
					if ((++addr_sp[TIMA_REG]) == 0)
					{
						addr_sp[IR_REG] |= 4;
						addr_sp[TIMA_REG] = addr_sp[TMA_REG];
					}
				}
			}
			div_ctrl += 4;
			ticks -= 4;
		}
		if (div_ctrl < div_tmp)
			addr_sp[DIV_REG]++;
	}

	else {
		div_ctrl += cur_tcks;
		if (div_ctrl < div_tmp)
			addr_sp[DIV_REG]++;
	}
}

static void
change_game()
{
	gb_vram_clks[0] = gb_vram_clks[1];
	gb_hblank_clks[0] = gb_hblank_clks[1];
	gb_oam_clks[0] = gb_oam_clks[1];
	gb_vbln_clks[0] = gb_vbln_clks[1];

	ime_flag = 1;
	hbln_dma_src = 0;
	hbln_dma_dst = 0;
	hbln_dma = 0;
	hdma_on = 0;
	cpu_cur_mode = 0;
	lcd_vbln_hbln_ctrl = 0;
	div_ctrl = 0;
	cpu_halt = 0;
	tac_on = 0;
	tac_counter = 0;
	tac_reload = 0;
	skip_next_frame = 0;
	nb_spr = 0;
	spr_cur_extr = 0;
	just_enabled = 0;
	write_is_delayed = 0;
	inst_is_cb = 0;

	regs_sets.regs[AF].UWord = 0;
	regs_sets.regs[BC].UWord = 0;
	regs_sets.regs[DE].UWord = 0;
	regs_sets.regs[SP].UWord = 0;
	regs_sets.regs[PC].UWord = 0;

	fullscr = 0;
	chg_gam = 1;
}

void
lcd_refrsh()
{
	static Uint8 hdma_tmp;
	cur_tcks >>= cpu_cur_mode;

	switch (addr_sp[LCDS_REG]&0x3) {
		case 0:
			gb_hblank_clks[0] -= cur_tcks;
			if (gb_hblank_clks[0] <= 0) {
				if (gboy_mode==1 && hdma_on==1) {
					int i;
					char *dma_src = (char *)hbln_dma_src;
					char *dma_dst = (char *)hbln_dma_dst;
					Uint16 *ptr_tmp;
					hdma_tmp = (addr_sp[VRAM_DMA]&0x7f)-1;
					addr_sp[VRAM_DMA] &= 0x80;
					addr_sp[VRAM_DMA] |= hdma_tmp;
					for (i=0; i<16; i++)
						*dma_dst++ = *dma_src++;
					ptr_tmp = (Uint16 *)(addr_sp+VRAM_DMA_SRC);
					*ptr_tmp = (addr_sp[VRAM_DMA_SRC+1] | (addr_sp[VRAM_DMA_SRC]<<8))+16;
					ptr_tmp = (Uint16 *)(addr_sp+VRAM_DMA_DST);
					*ptr_tmp = (addr_sp[VRAM_DMA_DST+1] | (addr_sp[VRAM_DMA_DST]<<8))+16;
					hbln_dma_dst += 16;
					hbln_dma_src += 16;
					if (--hbln_dma < 0)
						hdma_on=0, addr_sp[VRAM_DMA]=0xff;
				}
				addr_sp[LY_REG]++;
				addr_sp[LCDS_REG] &= ~LY_LYC_FLAG;
				if (addr_sp[LY_REG] == addr_sp[LYC_REG]) {
					addr_sp[LCDS_REG] |= LY_LYC_FLAG;
					if (addr_sp[LCDS_REG]&0x40)
						addr_sp[IR_REG] |= 2;
				}
				if (addr_sp[LY_REG] >= 144) {
					gb_vbln_clks[0] += gb_hblank_clks[0];
					gb_hblank_clks[0] = gb_hblank_clks[1];
					addr_sp[LCDS_REG] |= 1;
					lcd_vbln_hbln_ctrl = gb_vbln_clks[1]-gb_line_clks;
					if (addr_sp[LCDS_REG]&V_BLN_INT)
						addr_sp[IR_REG] |= 2;
					addr_sp[IR_REG] |= 1;
				}
				else {
					gb_oam_clks[0] += gb_hblank_clks[0];
					gb_hblank_clks[0] = gb_hblank_clks[1];
					addr_sp[LCDS_REG] |= 2;
					if (addr_sp[LCDS_REG]&OAM_INT)
						addr_sp[IR_REG] |= 2;
				}
			}
				break;
		case 1:
				gb_vbln_clks[0] -= cur_tcks;
				if (gb_vbln_clks[0] <= 0) {
					gb_oam_clks[0] += gb_vbln_clks[0];
					gb_vbln_clks[0] = gb_vbln_clks[1];
					addr_sp[LY_REG] = 0;
					addr_sp[LCDS_REG] &= ~LY_LYC_FLAG;
					if (addr_sp[LY_REG] == addr_sp[LYC_REG]) {
						addr_sp[LCDS_REG] |= LY_LYC_FLAG;
						if (addr_sp[LCDS_REG] & 0x40)
							addr_sp[IR_REG] |= 2;
					}
					addr_sp[LCDS_REG] &= ~3;
					addr_sp[LCDS_REG] |= 2;
					if ( (proc_evts()) == 1)
						change_game();
					if (!skip_next_frame)
						frame_update();
					skip_next_frame = frame_skip();
				}
				else {
					if (lcd_vbln_hbln_ctrl > gb_vbln_clks[0]) {
						lcd_vbln_hbln_ctrl -= gb_line_clks;
						addr_sp[LY_REG]++;
						addr_sp[LCDS_REG] &= ~LY_LYC_FLAG;
						if (addr_sp[LY_REG] == addr_sp[LYC_REG]) {
							addr_sp[LCDS_REG] |= LY_LYC_FLAG;
							if (addr_sp[LCDS_REG] & 0x40)
								addr_sp[IR_REG] |= 2;
						}
					}
				}
				break;
		case 2:
				gb_oam_clks[0] -= cur_tcks;
				if (gb_oam_clks[0] <= 0) {
					gb_vram_clks[0] += (gb_oam_clks[0]) + spr_extr_cycles[nb_spr>>3] + (addr_sp[SCR_X]&0x4);
					gb_oam_clks[0] = gb_oam_clks[1];
					spr_cur_extr = spr_extr_cycles[nb_spr>>3];
					addr_sp[LCDS_REG] |= 3;
				}
				break;
		case 3:
				gb_vram_clks[0] -= cur_tcks;
				if (gb_vram_clks[0] <= 0) {
					gb_hblank_clks[0] += gb_vram_clks[0];
					gb_hblank_clks[0] -= spr_cur_extr;
					gb_hblank_clks[0] -= (addr_sp[SCR_X]&0x4);
					gb_vram_clks[0] = gb_vram_clks[1];
					addr_sp[LCDS_REG] &= ~3;
					render_scanline(skip_next_frame);
					addr_sp[IR_REG] |= ((addr_sp[LCDS_REG]&H_BLN_INT)>>2);

				}
				break;
	}
}

void
proc_ints()
{
	Uint8 i = 1, indx = 0;
	Uint8 *ptr_stack;
	Uint16 *ptr_stack_wd;

	if (*pc == 0x76 || ime_flag == 1) {
		while ((!(i&addr_sp[IR_REG]) || !(i&addr_sp[IE_REG])) && (i <= 0x10))
			i <<= 1, indx++;
		/* We have interrupt */
		if (i <= 0x10) {
			if (*pc == 0x76)
				pc++, regs_sets.regs[PC].UWord++;
			if (ime_flag == 1) {
				ime_flag=0;
				addr_sp[IR_REG] &= ~i;
				regs_sets.regs[SP].UWord -= 2;
				ptr_stack = (Uint8 *)(regs_sets.regs[SP].UWord+(addr_sp_ptrs[regs_sets.regs[SP].UWord>>12]));
				ptr_stack_wd = (Uint16 *)ptr_stack;
				*ptr_stack_wd = (regs_sets.regs[PC].UWord); // XXX
				pc = (Uint8 *)(ints_offs[indx] + (addr_sp_ptrs[ints_offs[indx]>>12]));
				regs_sets.regs[PC].UWord = ints_offs[indx];


			}
		}
	}
}

/*
 * Execution routine.
 */
void
exec_next(int offset)
{
	static struct z80_set *rec;

	pc = addr_sp+offset;

	while (!chg_gam) {
		rec = z80_ldex + *pc;
		cur_tcks = rec->format[7];
		if (gbddb==1)
			gddb_main(0, pc, rec);
		if (rec->format[5] & DELAY) {
			execute_precise(rec);
		}
		else {
			do {
				if (inst_is_cb)
					rec = z80_ldex+256+*pc, cur_tcks = rec->format[7];
				inst_is_cb = 0;
				if (rec->format[5] & DELAY)
					execute_precise(rec);
				else
					rec->func(rec), timer_divider_update();
			} while (inst_is_cb == 1);
		}
		pc = (Uint8 *)(regs_sets.regs[PC].UWord+addr_sp_ptrs[(regs_sets.regs[PC].UWord)>>12]);
		proc_ints();
		if (addr_sp[LCDC_REG]&0x80)
			lcd_refrsh();
	}
	chg_gam = 0;
}

/*
 * Prepare to execute
 */
void
rom_exec(int offset)
{
	exec_next(offset);
}
