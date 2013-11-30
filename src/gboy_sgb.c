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
#define SGB_PACK_SIZE 16
#define SGB_MAX_PACK 7
#define IDLE 0
#define RESET 1
#define SENDING 2

Uint8 sgb_pack_buf[112];
long sgb_cur_bit=0;
long sgb_multi_player=0;
long sgb_four_players=0;
long sgb_next_ctrlr=0;
long sgb_cur_bit_shf=0;
long sgb_state=IDLE;
Uint32 sgb_scpal[512][4];

static void
sgb_att_blk()
{

}
static void
sgb_att_lin()
{

}
static void
sgb_att_div()
{

}
static void
sgb_att_chr()
{

}
static void
sgb_dummy()
{

}
static void
sgb_chr_trn()
{

}
static void
sgb_pic_trn()
{

}
static void
sgb_att_atf()
{

}
static void
sgb_dat_atf()
{

}
static void
sgb_win_msk()
{

}
static void
sgb_mul_req()
{
	if (sgb_pack_buf[1] & 1) {
		sgb_multi_player = 1;
		if (sgb_pack_buf[1] & 2)
			sgb_four_players = 1;
		else
			sgb_four_players = 0;
		sgb_next_ctrlr = 0x0e;
	}
	else {
		sgb_multi_player = 0;
		sgb_four_players = 0;
		sgb_next_ctrlr = 0x0f;
	}
}
static void
sgb_set_pal_scp()
{
	int i,j;
	Uint32 cur_col;
	Uint8 *t=&addr_sp[0x8800];
 
	for(i=0;i<512;i++) {
		for(j=0;j<4;j++,t+=2) {
			cur_col = (((*t&0x1f)<<3)<<16) | ((((*t>>5)&0x1f)<<3)<<8) | (((*t>>10)&0x1f)<<3);
			sgb_scpal[i][j]=cur_col;
		}
	}

}
static void
sgb_set_pal_ind()
{
	memcpy(pal_sgb[0],sgb_scpal[((sgb_pack_buf[2]&0x01)<<8)|sgb_pack_buf[1]],2*4);
	memcpy(pal_sgb[1],sgb_scpal[((sgb_pack_buf[4]&0x01)<<8)|sgb_pack_buf[3]],2*4);
	memcpy(pal_sgb[2],sgb_scpal[((sgb_pack_buf[6]&0x01)<<8)|sgb_pack_buf[5]],2*4);
	memcpy(pal_sgb[3],sgb_scpal[((sgb_pack_buf[8]&0x01)<<8)|sgb_pack_buf[7]],2*4);
}

static void
sgb_set_pal()
{
	int i, cur_col, num;
	Uint8 pal1, pal2;

	switch (sgb_pack_buf[0]>>3) {
		case 0:
			pal1 = 0;
			pal2 = 1;
			break;
		case 1:
			pal1 = 2;
			pal2 = 3;
			break;
		case 2:
			pal1 = 0;
			pal2 = 3;
			break;
		case 3:
			pal1 = 1;
			pal2 = 2;
			break;
	}

	for(i=0;i<4;i++) {
		num = (sgb_pack_buf[2]<<8)|sgb_pack_buf[1];
		cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
		pal_sgb[i][0]=cur_col;
	}
	num = (sgb_pack_buf[4]<<8)|sgb_pack_buf[3];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal1][1]=cur_col;

	num = (sgb_pack_buf[6]<<8)|sgb_pack_buf[5];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal1][2]=cur_col;

	num = (sgb_pack_buf[8]<<8)|sgb_pack_buf[7];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal1][3]=cur_col;
	
	num = (sgb_pack_buf[10]<<8)|sgb_pack_buf[9];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal2][1]=cur_col;


	num = (sgb_pack_buf[12]<<8)|sgb_pack_buf[11];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal2][2]=cur_col;

	num = (sgb_pack_buf[14]<<8)|sgb_pack_buf[13];
	cur_col = (((num&0x1f)<<3)<<16) | ((((num>>5)&0x1f)<<3)<<8) | (((num>>10)&0x1f)<<3);
	pal_sgb[pal2][3]=cur_col;
}

void (*sgb_cmds[0x19])() = { sgb_set_pal, sgb_set_pal, sgb_set_pal, sgb_set_pal, sgb_att_blk, sgb_att_lin, sgb_att_div, sgb_att_chr, sgb_dummy, sgb_dummy, sgb_set_pal_ind, sgb_set_pal_scp, sgb_dummy, sgb_dummy, sgb_dummy, sgb_dummy, sgb_dummy, sgb_mul_req, sgb_dummy, sgb_chr_trn, sgb_pic_trn, sgb_att_atf, sgb_dat_atf, sgb_win_msk, sgb_dummy };

static void
sgb_reset()
{
	memset(sgb_pack_buf, 0, SGB_PACK_SIZE*SGB_MAX_PACK);
	sgb_cur_bit=0;
	sgb_cur_bit_shf=0;
	sgb_state=RESET;
}

static void
sgb_is_sending(Uint8 val)
{
	switch (val) {
		case 0x10:
			sgb_cur_bit=1;
			break;
		case 0x20:
			sgb_cur_bit=0;
			break;
		case 0x30:
			if (((sgb_cur_bit_shf % 128) == 0) && sgb_cur_bit_shf != 0) {
				if ((sgb_cur_bit_shf / 128) == (sgb_pack_buf[0]&7)) {
					(sgb_cmds[sgb_pack_buf[0]>>3])();
					sgb_state=IDLE; // Shut down
				}
			}
			else {
				sgb_pack_buf[sgb_cur_bit_shf/8] |= sgb_cur_bit << (sgb_cur_bit_shf%8);
				sgb_cur_bit_shf++;
			}
	}
}

static void
sgb_is_reset(Uint8 val)
{
	switch (val) {
		case 0x30:
			sgb_state = SENDING;
			break;
		default:
			sgb_state = IDLE;
	}
}

static void
sgb_is_idle(Uint8 val)
{
	if (val == 0x30)
		; // Multiplayer stuff
}

void
do_sgb_packet(Uint8 val)
{
	val &= 0x30; // P14 and P15 bits

	/* A 0 value will always leave SGB in RESET state */
	if (val == 0)
		sgb_reset();
	else {
		switch (sgb_state) {
			case IDLE:
				sgb_is_idle(val);
				break;
			case RESET:
				sgb_is_reset(val);
				break;
			case SENDING:
				sgb_is_sending(val);
				break;

		}
	}

}

Uint8
sgb_read()
{
	Uint8 val, joy;

	sgb_state = IDLE;

	//gbSgbReadingController |= 4;

	val = addr_sp[0xff00] & 0x30;

	if (val == 0x20 || val == 0x10) {
		val &= 0xf0;
		if (sgb_multi_player) {
			switch (sgb_next_ctrlr) {
				case 0x0f:
		          joy = 0;
		          break;
				case 0x0e:
		          joy = 1;
		          break;
				case 0x0d:
		          joy = 2;
		          break;
				case 0x0c:
		          joy = 3;
		          break;
				default:
		          joy = 0;
		          break;
			}
		}
		addr_sp[0xff00] = val;
	}
	else {
		if (sgb_multi_player)
			addr_sp[0xff00] = 0xf0 | sgb_next_ctrlr;
		else
			addr_sp[0xff00] = 0xff;
	}

	return addr_sp[0xff00];
}
