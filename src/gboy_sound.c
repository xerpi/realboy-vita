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

#include "gboy.h"
extern SDL_AudioSpec desired;
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

void
sound_update()
{

	Sint32 i;
	samples = samp_cnt*desired.samples;
	
	while (1)
	{
		left=right=0;
		if (sqwave[0].on==1)
		{
			sample=(Sint32)(sqwave[0].cur_env);
			if ((sqwave[0].sample_dut_cnt-=samp_cnt)<=0)
			{
				sqwave[0].sample_dut_cnt=sqwave[0].sample_cnt+samp_cnt+5;
				sqwave[0].sig=-sqwave[0].sig;
			}
			if ((sqwave[0].sample_cnt-=samp_cnt)<=0)
			{
				sqwave[0].sig=-sqwave[0].sig;
				sqwave[0].sample_cnt+=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[0].frq)<<5)));
				sqwave[0].sample_dut_cnt+=sqwave[0].sample_cnt/sqwave[0].duts[sqwave[0].dut];
			}
	
			if (sqwave[0].mode==1 && sqwave[0].len)
			{
				if ((sqwave[0].len_cnt-=samp_cnt)<=0)
				{
					sqwave[0].len_cnt+=(gb_clk_rate/256);
					if ((--sqwave[0].len)<=0)
					{
						sqwave[0].on=0;
						addr_sp[0xff26]&=0xfe;
						//goto sound2;
					}
				}
			}
			if (sqwave[0].env)
			{
				if ((sqwave[0].env_cnt-=samp_cnt)<=0)
				{
					sqwave[0].env_cnt+=(gb_clk_rate/64);
					if ((--sqwave[0].env)<=0)
					{
						sqwave[0].env=(addr_sp[0xff12]&0x7);
						if (sqwave[0].env_dir==0)
						{
							if ((--sqwave[0].cur_env)<=0)
								sqwave[0].cur_env=0;
						}
						else
						{
							if ((++sqwave[0].cur_env)>15)
								sqwave[0].cur_env=15;
						}
					}
				}
			}
			if (sqwave[0].swp)
			{
				if ((sqwave[0].swp_cnt-=samp_cnt)<=0)
				{
					sqwave[0].swp_cnt+=(gb_clk_rate/128);
					if ((--sqwave[0].swp)<=0)
					{
						sqwave[0].swp=((addr_sp[0xff10]&0x70)>>4);
						if (sqwave[0].swp_dir==0)
						{
							sqwave[0].frq+=(sqwave[0].frq>>sqwave[0].swp_shf);
							if (sqwave[0].frq>=2048)
								sqwave[0].frq=2047;
						}
						else
						{
							sqwave[0].frq-=(sqwave[0].frq>>sqwave[0].swp_shf);
							if (sqwave[0].frq<=0)
							{
								sqwave[0].on=0;
								addr_sp[0xff26]&=0xfe;
								//goto sound2;
							}
						}
						sqwave[0].sample_cnt+=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[0].frq)<<5)));
						sqwave[0].sample_dut_cnt+=(sqwave[0].sample_cnt/sqwave[0].duts[sqwave[0].dut]);
					}
				}
			}
			if (sqwave[0].lft)
			{
				left=sample;
				if (sqwave[0].sig==-1)
					left=-left;
			}
			if (sqwave[0].rgh)
			{
				right=sample;
				if (sqwave[0].sig==-1)
					right=-right;
			}
		}
sound2:;
		if (sqwave[1].on==1)
		{
			if ((sqwave[1].sample_dut_cnt-=samp_cnt)<=0)
			{
				sqwave[1].sample_dut_cnt=sqwave[1].sample_cnt+samp_cnt+1;
				sqwave[1].sig=-sqwave[1].sig;
			}
			if ((sqwave[1].sample_cnt-=samp_cnt)<=0)
			{
				sqwave[1].sig=-sqwave[1].sig;
				sqwave[1].sample_cnt+=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[1].frq)<<5)));
				sqwave[1].sample_dut_cnt+=sqwave[1].sample_cnt/sqwave[1].duts[sqwave[1].dut];
			}
	
			if (sqwave[1].mode==1 && sqwave[1].len)
			{
				if ((sqwave[1].len_cnt-=samp_cnt)<=0)
				{
					sqwave[1].len_cnt+=(gb_clk_rate/256);
					if ((--sqwave[1].len)<=0)
					{
						sqwave[1].on=0;
						addr_sp[0xff26]&=0xfd;
						//goto sound3;
					}
				}
			}
			if (sqwave[1].env)
			{
				if ((sqwave[1].env_cnt-=samp_cnt)<=0)
				{
					sqwave[1].env_cnt+=(gb_clk_rate/64);
					if ((--sqwave[1].env)<=0)
					{
						sqwave[1].env=(addr_sp[0xff17]&0x7);
						if (sqwave[1].env_dir==0)
						{
							if ((--sqwave[1].cur_env)<=0)
								sqwave[1].cur_env=0;
						}
						else
						{
							if ((++sqwave[1].cur_env)>15)
								sqwave[1].cur_env=15;
						}
					}
				}
			}
			if (sqwave[1].lft)
			{
				left=(sqwave[1].sig==1)?(left+sqwave[1].cur_env):(left-sqwave[1].cur_env);
			}
			if (sqwave[1].rgh)
			{
				right=(sqwave[1].sig==1)?(right+sqwave[1].cur_env):(right-sqwave[1].cur_env);
			}
		}
sound3:;
sound4:;
		if (noise.on==1)
		{
			if (noise.freq)
			{
				if ((noise.sample_cnt-=samp_cnt)<=0)
				{
					noise.sample_cnt += gb_clk_rate/noise.freq;
					noise.mask = (((noise.shf_reg&2)>>1)^(noise.shf_reg&1))<<(noise.shf_wid?6:14);
					noise.shf_reg >>= 1;
					noise.shf_reg |= noise.mask;
					noise.shf_reg &= (noise.shf_wid?0x7f:0x7fff);
					noise.sig = noise.shf_reg;
				}
			}
			if (noise.mode==1 && noise.len)
			{
				if ((noise.len_cnt-=samp_cnt)<=0)
				{
					noise.len_cnt+=(gb_clk_rate/256);
					if ((--noise.len)<=0)
					{
						noise.on=0;
						addr_sp[0xff26]&=0xf7;
						//goto end_sound;
					}
				}
			}
			if (noise.env)
			{
				if ((noise.env_cnt-=samp_cnt)<=0)
				{
					noise.env_cnt+=(gb_clk_rate/64);
					if ((--noise.env)<=0)
					{
						noise.env=(addr_sp[0xff21]&0x7);
						if (noise.env_dir==0)
						{
							if ((--noise.cur_env)<=0)
							{
								noise.cur_env=0;
								noise.on=0;
								//goto end_sound;
							}
						}
						else
						{
							if ((++noise.cur_env)>15)
								noise.cur_env=15;
						}
					}
				}
			}
			if (noise.lft)
			{
				left+=((noise.sig&noise.cur_env)<<1);
			}
			if (noise.rgh)
			{
				right+=((noise.sig&noise.cur_env)<<1);
			}
		}
end_sound:;
		samples-=samp_cnt;
		left*=snd_ctrl.lft_lvl;
		left<<=6;
		if (left>32767) left=32767;
		if (left<-32767) left=-32767;
		right*=snd_ctrl.rgh_lvl;
		right<<=6;
		if (right>32767) right=32767;
		if (right<-32767) right=-32767;
		playbuf[buf_pos++] = (Sint16)left;
		playbuf[buf_pos++] = (Sint16)right;
		if (buf_pos >= (buf_siz/2))
		{
			buf_pos=0;
			buf_full=1;
			return;
		}
	}
}

void
read_sound_reg()
{

}

void
write_sound_reg(Uint8 reg, Uint8 val)
{

	SDL_LockAudio();

	switch (reg) {
		case 0x10:
			sqwave[0].swp=(val&0x70)>>4;
			sqwave[0].swp_dir=(val&0x8)>>3;
			sqwave[0].swp_shf=(val&0x7);
			addr_sp[reg+0xff00]=(val);
			break;
		case 0x11:
			sqwave[0].dut=(val&0xc0)>>6;
			sqwave[0].len=(64-(val&0x3f));
			sqwave[0].sample_dut_cnt=sqwave[0].sample_cnt/sqwave[0].duts[sqwave[0].dut];
			addr_sp[reg+0xff00]=val;
			break;
		case 0x12:
			sqwave[0].init_env=(val&0xf0)>>4;
			sqwave[0].cur_env=sqwave[0].init_env;
			sqwave[0].env_dir=(val&0x8)>>3;
			sqwave[0].env=(val&0x7);
			addr_sp[reg+0xff00]=val;
			break;
		case 0x13:
			addr_sp[reg+0xff00]=val;
			sqwave[0].frq=(addr_sp[0xff13])|(((Uint16)addr_sp[0xff14]&0x7)<<8);
			sqwave[0].sample_cnt=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[0].frq)<<5)));
			sqwave[0].sample_dut_cnt=sqwave[0].sample_cnt/sqwave[0].duts[sqwave[0].dut];
			break;
		case 0x14:
			addr_sp[reg+0xff00]=(val);
			sqwave[0].mode=(val&0x40)>>6;
			sqwave[0].frq=(addr_sp[0xff13])|((((Uint16)addr_sp[0xff14])&0x7)<<8);
			sqwave[0].sample_cnt=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[0].frq)<<5)));
			sqwave[0].sample_dut_cnt=sqwave[0].sample_cnt/sqwave[0].duts[sqwave[0].dut];
			if (val&0x80)
			{
				sqwave[0].on=1;
				sqwave[0].len=(64-(addr_sp[0xff11]&0x3f));
				sqwave[0].init_env=(addr_sp[0xff12]&0xf0)>>4;
				sqwave[0].env=(addr_sp[0xff12]&0x7);
				sqwave[0].swp=(addr_sp[0xff10]&0x70)>>4;
				sqwave[0].sig=1;
				addr_sp[0xff26] |= 1;
			}
			break;
		case 0x16:
			sqwave[1].dut=(val&0xc0)>>6;
			sqwave[1].len=(64-(val&0x3f));
			sqwave[1].sample_dut_cnt=sqwave[1].sample_cnt/sqwave[1].duts[sqwave[1].dut];
			addr_sp[reg+0xff00]=val;
			break;
		case 0x17:
			sqwave[1].init_env=(val&0xf0)>>4;
			sqwave[1].cur_env=sqwave[1].init_env;
			sqwave[1].env_dir=(val&0x8)>>3;
			sqwave[1].env=(val&0x7);
			addr_sp[reg+0xff00]=val;
			break;
		case 0x18:
			addr_sp[reg+0xff00]=val;
			sqwave[1].frq=(addr_sp[0xff18])|(((Uint16)addr_sp[0xff19]&0x7)<<8);
			sqwave[1].sample_cnt=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[1].frq)<<5)));
			sqwave[1].sample_dut_cnt=sqwave[1].sample_cnt/sqwave[1].duts[sqwave[1].dut];
			break;
		case 0x19:
			addr_sp[reg+0xff00]=(val);
			sqwave[1].mode=(val&0x40)>>6;
			sqwave[1].frq=(addr_sp[0xff18])|((((Uint16)addr_sp[0xff19])&0x7)<<8);
			sqwave[1].sample_cnt=(gb_clk_rate/(gb_clk_rate/((2048-sqwave[1].frq)<<5)));
			sqwave[1].sample_dut_cnt=sqwave[1].sample_cnt/sqwave[1].duts[sqwave[1].dut];
			if (val&0x80)
			{
				sqwave[1].on=1;
				sqwave[1].len=(64-(addr_sp[0xff16]&0x3f));
				sqwave[1].init_env=(addr_sp[0xff17]&0xf0)>>4;
				sqwave[1].env=(addr_sp[0xff17]&0x7);
				sqwave[1].sig=1;
				addr_sp[0xff26] |= 2;
			}
			break;
		case 0x20:
			noise.len=(64-(val&0x3f));
			addr_sp[reg+0xff00]=val;
			break;
		case 0x21:
			noise.init_env=(val&0xf0)>>4;
			noise.cur_env=noise.init_env;
			noise.env_dir=(val&0x8)>>3;
			noise.env=(val&0x7);
			addr_sp[reg+0xff00]=val;
			break;
		case 0x22:
			addr_sp[reg+0xff00]=val;
			noise.shf_clk_frq=(((val>>4)&0xf)+1);
			noise.shf_frq = (1<<noise.shf_clk_frq); // input to the PRNG
			noise.shf_wid=((val&0x8)>>3);
			/* Input to the shift counter */
			switch ((noise.div_rat=(val&0x7)))
			{
				case 0:
					noise.div_rat_cnt = gb_clk_rate/1048576;
					break;
				case 1:
					noise.div_rat_cnt = gb_clk_rate/524288;
					break;
				case 2:
					noise.div_rat_cnt = gb_clk_rate/370727;
					break;
				case 3:
					noise.div_rat_cnt = gb_clk_rate/262144;
					break;
				case 4:
					noise.div_rat_cnt = gb_clk_rate/220435;
					break;
				case 5:
					noise.div_rat_cnt = gb_clk_rate/185363;
					break;
				case 6:
					noise.div_rat_cnt = gb_clk_rate/155871;
					break;
				case 7:
					noise.div_rat_cnt = gb_clk_rate/131072;
					break;
			}
			noise.freq = ((1048576/(noise.div_rat+1))/(1<<noise.shf_clk_frq));
			noise.sample_cnt = gb_clk_rate/noise.freq;
			break;
		case 0x23:
			addr_sp[reg+0xff00]=(val);
			noise.mode=(val&0x40)>>6;
			if (val&0x80)
			{
			//if (!noise.on)
			//	noise.sample_cnt = gb_clk_rate/noise.freq;
				noise.on=1;
				addr_sp[0xff26] |= 0x8;
				noise.shf_reg=0xff;
				noise.sig = rand();
				noise.env=(addr_sp[0xff21]&0x7);
				noise.init_env=(addr_sp[0xff21]&0xf0)>>4;
				noise.shf_reg=0x7fff;
			}
			break;
		case 0x24:
			snd_ctrl.rgh_lvl=val&0x7;
			snd_ctrl.lft_lvl=(val&0x70)>>4;
			addr_sp[reg+0xff00]=val;
			break;
		case 0x25:
			sqwave[0].lft=(val&0x10)>>4;
			sqwave[0].rgh=(val&0x1);
			sqwave[1].lft=(val&0x20)>>4;
			sqwave[1].rgh=(val&0x2);
			noise.lft = (val&0x80)>>7;
			noise.rgh = (val&0x8)>>3;
			addr_sp[reg+0xff00]=val;
			break;
		case 0x26:
			snd_ctrl.snd_on = (val & 0x80) ? 1 : 0;
			//addr_sp[reg+0xff00]= val;
			break;
		default:
			addr_sp[reg+0xff00]=val;
	}

	SDL_UnlockAudio();
}
void
update_stream(void *userdata,Uint8 *stream,int snd_len)
{

		while (buf_full==0) {
			//if (snd_ticks>=samp_cnt)
				sound_update();
			//else
			//	break;
		}
		buf_full=0;
		memcpy(stream, playbuf, snd_len);
		memset(playbuf, 0, snd_len);
}


static void
init_gb_snd()
{
	Uint8 i;

//for (i=0x10; i<0x40; i++)
//	write_sound_reg(i, addr_sp[i]);

	/* These timers are clocked by the main CPU */
	for (i=0; i<2; i++) {
		sqwave[i].len_cnt=(gb_clk_rate/256); // 16384 ticks for GB
		sqwave[i].env_cnt=(gb_clk_rate/64);
		sqwave[i].swp_cnt=(gb_clk_rate/128);
		sqwave[i].len=0;
		sqwave[i].lft=1;
		sqwave[i].rgh=1;
		sqwave[i].on=0;
		sqwave[i].sig=1;
		sqwave[i].duts[0]=8;
		sqwave[i].duts[1]=4;
		sqwave[i].duts[2]=2;
		sqwave[i].duts[3]=1.333;
	}
	samp_cnt=((gb_clk_rate)/(samp_rate));

	noise.shf_reg=0xff;
	noise.len_cnt=(gb_clk_rate/256); // 16384 ticks for GB
	noise.env_cnt=(gb_clk_rate/64);
}

void
snd_reset()
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	free(playbuf);
}

void
snd_start()
{
	SDL_Init(SDL_INIT_AUDIO);
	desired.freq = 44100;
	desired.samples = 2048;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.callback = update_stream;
	desired.userdata = NULL;

	/* Open audio */
	if (!SDL_OpenAudio(&desired,NULL)) {
		samp_rate = desired.freq;
		buf_siz = desired.size;
		init_gb_snd();
		playbuf=(Sint16 *)malloc(desired.size+1);
		memset(playbuf,0,desired.size);
		SDL_PauseAudio(0);
	} 
}
