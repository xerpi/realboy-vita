/* RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
 * Game Boy sound emulation (c) Anthony Kruize (trandor@labyrinth.net.au)
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
struct {
	int freq;
	Uint16 format;
	Uint8 channels;
	Uint8 silence;
	Uint16 samples;
	Uint32 size;
	void (*callback)(void *userdata, Uint8 *stream, int len);
	void *userdata;
} desired;
Sint16 *playbuf;
double freq_tbl[2048];
double freq_tbl_snd3[2048];
double freq_tbl_snd4[256];
static Sint32 sample, left, right;
static long samples=0;

struct sqwave {
	Uint32 frq;
	Sint32 sample_cnt; // sample counter
	Sint32 sample_dut_cnt; // sample's duty counter
	Uint32 swp; // value from register
	Sint32 swp_cnt; // counter
	Uint32 swp_shf; // counter
	Uint32 swp_dir; // counter
	Uint32 len;
	Sint32 len_cnt;
	Uint32 stp_cnt;
	Uint32 dut;
	Uint32 env;
	Sint32 env_cnt;
	Sint32 env_dir;
	Uint32 indx; // 2048-frequency index
	Uint32 mode; // 2048-frequency index
	Uint32 reinit;
	Uint32 lft;
	Uint32 rgh;
	Uint32 on;
	Sint32 sig;
	Uint32 init_env;
	Sint32 cur_env;
	float duts[4];
} sqwave[2];

struct snd_ctrl {
	Uint32 snd_on;
	Uint32 rgh_lvl;
	Uint32 lft_lvl;
} snd_ctrl;

struct noise {
	Sint32 cur_env;
	Sint32 sample_cnt;
	Sint32 len_cnt;
	Uint32 len;
	Uint32 init_env;
	Uint32 env;
	Sint32 env_cnt;
	Sint32 env_dir;
	Uint32 on;
	Uint32 mode;
	Uint32 shf_clk_frq;
	Uint32 shf_wid;
	Uint32 lft;
	Sint32 sig;
	Uint32 rgh;
	Uint32 div_rat;
	Uint32 div_rat_cnt;
	Uint32 shf_frq;
	Uint32 freq;
	Uint32 mask;
	Sint16 shf_reg;
} noise;

Uint32 buf_full=0;
Uint32 buf_pos=0;
Uint32 buf_siz;
Uint32 samp_rate;
Sint32 samp_cnt;
