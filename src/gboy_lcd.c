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

#include <math.h>
#include "gboy.h"

static Uint8 back_col[170][170];

/* Global/Exported variables */
Uint32 spr_pal_inc_indx;
Uint32 spr_pal_cur_indx;
Uint32 pal_inc_indx;
Uint32 pal_cur_indx;
Uint16 spr_pal[8][4];
Uint16 bg_pal[8][4];

/* Imported/External variables. Defined in gboy_video.c */
extern SDL_Surface *back;

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

static void
render_win_cgb(Uint32 *buf)
{
	Sint32 pal;
	Sint16 y, x, i, x1=0;
	Sint16 tile_num;
	Uint8 tile_att;
	Sint8 sum;
	Uint8 *tile_map, *tile_ptr, color, shftr, *ptr_att, pri_att;

	/* Point to tile map */
	if (addr_sp[0xff40]&0x40)
		tile_map=addr_sp+0x9c00, ptr_att=gb_cart.cart_vram_bank+0x1c00;
	else
		tile_map=addr_sp+0x9800, ptr_att=gb_cart.cart_vram_bank+0x1800;

	/* Advance to row in the tile map */
	tile_map +=(((addr_sp[0xff44]-addr_sp[0xff4a])>>3)<<5);
	ptr_att +=(((addr_sp[0xff44]-addr_sp[0xff4a])>>3)<<5);

	/* Offset to row and column within tile */
	y = (((addr_sp[0xff44]-addr_sp[0xff4a])&0x7));
	x = ((addr_sp[0xff4b]-7)<0 ? 0 : addr_sp[0xff4b]-7);

	for (i=0; x<160; x+=8, i++) {
		tile_num = tile_map[i];
		tile_att = ptr_att[i];
		pal = tile_att&0x7;
		tile_num = (!(addr_sp[0xff40]&0x10) ? 256 + (Sint8)tile_num : tile_num);
		/* Pointer to tile */
		tile_ptr = ((tile_att&0x8) ? gb_cart.cart_vram_bank+(tile_num<<4) : addr_sp+0x8000+(tile_num<<4));
		/* Advance to row within tile */
		tile_ptr += (!(tile_att&0x40) ? y<<1 : ((7-y)<<1));
		if ((tile_att&0x20)) shftr=0, sum=1;
		else shftr=7, sum=-1;
		for (x1=0; x1<8 && (x+x1)<160; x1++, shftr+=sum) 
		{
			color = ((tile_ptr[0]>>shftr)&1)|((((tile_ptr[1]>>shftr))&1)<<1);
			buf[x+x1] = pal_color[(bg_pal[pal][color])&0x7fff];
			back_col[x+x1][addr_sp[0xff44]]=pri_att+color;
		}
	}
}

static void
render_win(Uint32 *buf)
{
	Sint16 y, x, i, x1=0;
	Sint16 tile_num;
	Uint8 *tile_map, *tile_ptr, color, shftr;
	
	/* Pointer to map */
	if (addr_sp[0xff40]&0x40)
		tile_map=&addr_sp[0x9c00];
	else
		tile_map=&addr_sp[0x9800];

	/* Advance to row in tile map */
	tile_map +=(((addr_sp[0xff44]-addr_sp[0xff4a])>>3)<<5);
	/* Offset to row within tile */
	y = (((addr_sp[0xff44]-addr_sp[0xff4a])&0x7)<<1);

	/* Offset to column within tile */
	x = ((addr_sp[0xff4b]-7) < 0 ? 0 : addr_sp[0xff4b]-7);

	for (i=0; x<160; x+=8, i++) {
		tile_num = tile_map[i];
		if (!(addr_sp[0xff40]&0x10))
			tile_num=256+(signed char)tile_num;
		/* Pointer to tile */
		tile_ptr = &addr_sp[0x8000+(tile_num<<4)];
		/* Advance to row within tile */
		tile_ptr += y;
		for (shftr=7, x1=0; x1<8 && (x+x1)<160; x1++, shftr--) {
			color = ((tile_ptr[0]>>shftr)&1)|((((tile_ptr[1]>>shftr))&1)<<1);
			buf[x+x1] = pal_grey[(addr_sp[0xff47]>>(color<<1))&3];
			back_col[x+x1][addr_sp[0xff44]]=color;
		}
	}
}

static void
render_spr_cgb(Uint32 *buf, struct spr_attr *sprite)
{
	Uint8 *tile_ptr;
	Uint8 xx;
	Uint8 color, shftr;
	Sint8 sum;
	
	/* Pointer to tile */
	if (sprite->page)
		tile_ptr=gb_cart.cart_vram_bank+(sprite->tile_num<<4);
	else
		tile_ptr=&addr_sp[0x8000+(sprite->tile_num<<4)];
	
	/* Pointer to row in tile */
	if (!sprite->yflip)
		tile_ptr+=((sprite->yoff)<<1);
	else
		tile_ptr+=(sprite->sizey-1-sprite->yoff)<<1;
	
	xx=sprite->xoff;
	if (sprite->xflip)
		shftr=xx, sum=1;
	else
		shftr=7-xx, sum=-1;
	for(; xx<8; xx++, shftr+=sum) 
	{
		color = ((tile_ptr[0]>>shftr)&1)|((((tile_ptr[1]>>shftr))&1)<<1);
		if (color)
		{
			if (addr_sp[0xff40]&0x1)
			{
				if ((!(sprite->priority)) || (!((back_col[sprite->x+xx][addr_sp[0xff44]])&0x0f)))
					buf[sprite->x+xx] = pal_color[(spr_pal[sprite->pal_col][color])&0x7fff];
			}
			else
				buf[sprite->x+xx] = pal_color[(spr_pal[sprite->pal_col][color])&0x7fff];
		}
	}
}

static void
render_spr_dmg(Uint32 *buf, struct spr_attr *sprite)
{
	Uint8 *tile_ptr;
	Uint8 xx;
	Uint8 color, shftr;
	Sint8 sum;
	
	/* Pointer to tile */
	tile_ptr=&addr_sp[0x8000+(sprite->tile_num<<4)];
	
	/* Pointer to row in tile */
	if (!sprite->yflip)
		tile_ptr+=((sprite->yoff)<<1);
	else 
		tile_ptr+=(sprite->sizey-1-sprite->yoff)<<1;
	
	xx=sprite->xoff;
	if (sprite->xflip)
		shftr=xx, sum=1;
	else
		shftr=7-xx, sum=-1;
	for (; xx<8; xx++, shftr+=sum) {
		if (!(sprite->priority) || !(back_col[sprite->x+xx][addr_sp[0xff44]]))
		{
			if ((color = ((tile_ptr[0]>>shftr)&1)|((((tile_ptr[1]>>shftr))&1)<<1)))
				buf[sprite->x+xx] = pal_grey[(addr_sp[0xff48+sprite->pal]>>(color<<1))&3];
		}
	}
}

static void
render_spr()
{
	Sint16 tile_num, x, y, attr;
	Uint8 i, yoff, xoff, sizey;
	Uint8 *oam_ptr = &addr_sp[0xfe00];

	nb_spr = 0;

	if (addr_sp[0xff40]&4)
		sizey = 16;
	else
		sizey = 8;

	for (i=0;i<40;i++) {
		if ((addr_sp[0xff44]>=(oam_ptr[0]-16)) && (addr_sp[0xff44]-(oam_ptr[0]-16)<sizey) 
			&& (((oam_ptr[1]-8)<160))) 
		{
			spr_attr[nb_spr].sizey=sizey;
			spr_attr[nb_spr].x=oam_ptr[1]-8;
			spr_attr[nb_spr].y=oam_ptr[0]-16;
			spr_attr[nb_spr].xoff= (oam_ptr[1] < 8 ? 8-oam_ptr[1] : 0);
			spr_attr[nb_spr].yoff=addr_sp[0xff44]-(oam_ptr[0]-16);
			spr_attr[nb_spr].xflip=(oam_ptr[3]&0x20)>>5;
			spr_attr[nb_spr].yflip=(oam_ptr[3]&0x40)>>6;
			spr_attr[nb_spr].pal_col=(oam_ptr[3]&0x07);
			spr_attr[nb_spr].pal= (oam_ptr[3]&0x10 ? 1 : 0);
			spr_attr[nb_spr].page=(oam_ptr[3]&0x08)>>3;
			spr_attr[nb_spr].priority=(oam_ptr[3]&0x80);
			spr_attr[nb_spr].tile_num= (sizey==16 ? oam_ptr[2]&0xfe : oam_ptr[2]);
			if (++nb_spr>10) {
				nb_spr=10;
				return;
			}
		}
		oam_ptr += 4;
	}
}

static void
render_back_cgb(Uint32 *buf)
{
	int i, j;
	Sint16 tile_num, sum;
	Uint8 indx, x, y, x1, wbit, tile_att, pal_num, shftr;
	Uint8 *ptr_data, *ptr_map, *ptr_att;
	
	/* Point to tile map */
	if (addr_sp[0xff40]&0x8)
		ptr_map=addr_sp+0x9c00, ptr_att=gb_cart.cart_vram_bank+0x1c00;
	else
		ptr_map=addr_sp+0x9800, ptr_att=gb_cart.cart_vram_bank+0x1800;
	
	/* Current line + SCROLL Y */
	y = addr_sp[0xff44]+addr_sp[0xff42];
	/* SCROLL X */
	x = addr_sp[0xff43];
	x1 = x>>3;
	
	/* Advance to row in tile map */
	ptr_map += ((y>>3)<<5)&0x3ff;
	ptr_att += ((y>>3)<<5)&0x3ff;
	
	x &= 7; // bit offset
	j = x;
	for (i=0; x<168; x+=8, x1++) {
		tile_num = ptr_map[x1&0x1f];
		tile_att = ptr_att[x1&0x1f];
		pal_num = tile_att&0x7;
		tile_num = (!(addr_sp[0xff40]&0x10) ? 256 + (Sint8)tile_num : tile_num);
		ptr_data = (tile_att&0x8 ? gb_cart.cart_vram_bank+(tile_num<<4) : addr_sp+0x8000+(tile_num<<4));
		ptr_data += (!(tile_att&0x40) ? ((y&0x7)<<1) : ((7-(y&0x7))<<1));
		if ((tile_att&0x20))
			shftr=j, sum=1;
		else
			shftr=7-j, sum=-1;
		for (; j<8; j++, shftr+=sum) {
			indx = ((ptr_data[0]>>shftr)&1)|((((ptr_data[1]>>shftr))&1)<<1);
			buf[i] = pal_color[(bg_pal[pal_num][indx])&0x7fff];
			back_col[i][addr_sp[0xff44]]=(tile_att&0x80)+indx;
			i++;
		}
		j=0;
	}

}

static void
render_back_sgb(Uint32 *buf)
{
	int i, j;
	Uint8 *ptr_data;
	Uint8 *ptr_map;
	Uint8 indx, shftr, x, y, x1;
	Sint16 tile_num;
	
	/* Point to tile map */
	if (addr_sp[0xff40]&0x8)
		ptr_map=addr_sp+0x9c00;
	else
		ptr_map=addr_sp+0x9800;
	
	/* Current line + SCROLL Y */
	y = addr_sp[0xff44]+addr_sp[0xff42];
	/* SCROLL X */
	j = addr_sp[0xff43];
	x1 = j>>3;
	
	/* Advance to row in tile map */
	ptr_map += ((y>>3)<<5)&0x3ff;
	
	i=0;
	j &= 7;
	x = 8-j;
	shftr=((Uint8)(~j))%8; // shift factor
	for (; x<168; x+=8) {
		tile_num = ptr_map[x1++&0x1f];
		if (!(addr_sp[0xff40]&0x10))
			tile_num = 256 + (signed char)tile_num;
		ptr_data = addr_sp+0x8000+(tile_num<<4); // point to tile; each tile is 8*8*2=128 bits=16 bytes
		ptr_data+=(y&7)<<1; // point to row in tile depending on LY and SCROLL Y; each row is 8*2=16 bits=2 bytes
		for (; j<8 && (x+j)<168; shftr--, j++) {
			indx = ((ptr_data[0]>>shftr)&1)|((((ptr_data[1]>>shftr))&1)<<1);
			buf[i] = pal_sgb[sgb_pal_map[x/8][addr_sp[0xff44]/8]][(addr_sp[0xff47]>>(indx<<1))&3];
			back_col[i][addr_sp[0xff44]]=indx;
			i++;
		}
		j=0;
		shftr=7;
	}
}

static void
render_back(Uint32 *buf)
{
	int i, j;
	Uint8 *ptr_data;
	Uint8 *ptr_map;
	Uint8 indx, shftr, x, y, x1;
	Sint16 tile_num;
	
	/* Point to tile map */
	if (addr_sp[0xff40]&0x8)
		ptr_map=addr_sp+0x9c00;
	else
		ptr_map=addr_sp+0x9800;
	
	/* Current line + SCROLL Y */
	y = addr_sp[0xff44]+addr_sp[0xff42];
	/* SCROLL X */
	j = addr_sp[0xff43];
	x1 = j>>3;
	
	/* Advance to row in tile map */
	ptr_map += ((y>>3)<<5)&0x3ff;
	
	i=0;
	j &= 7;
	x = 8-j;
	shftr=((Uint8)(~j))%8; // shift factor
	for (; x<168; x+=8) {
		tile_num = ptr_map[x1++&0x1f];
		if (!(addr_sp[0xff40]&0x10))
			tile_num = 256 + (signed char)tile_num;
		ptr_data = addr_sp+0x8000+(tile_num<<4); // point to tile; each tile is 8*8*2=128 bits=16 bytes
		ptr_data+=(y&7)<<1; // point to row in tile depending on LY and SCROLL Y; each row is 8*2=16 bits=2 bytes
		for (; j<8 && (x+j)<168; shftr--, j++) {
			indx = ((ptr_data[0]>>shftr)&1)|((((ptr_data[1]>>shftr))&1)<<1);
			buf[i] = pal_grey[(addr_sp[0xff47]>>(indx<<1))&3];
			back_col[i][addr_sp[0xff44]]=indx;
			i++;
		}
		j=0;
		shftr=7;
	}
}

void
render_scanline(long skip)
{
	if (skip)
		return;

	int i;

	/* Buffer where actual pixels are stored */
	Uint32 *buf=(Uint32 *)back->pixels+(addr_sp[0xff44]*(back->pitch>>2));

	if ((addr_sp[0xff40]&1))
	{
		if (gboy_mode==DMG)
  			render_back(buf);
		else if (gboy_mode==CGB)
			render_back_cgb(buf);
		else
			render_back_sgb(buf);
	}

	/* Window must be enabled and visible */
	if ((addr_sp[0xff40]&0x20) && ((addr_sp[0xff4b]) < 166) && (addr_sp[0xff44] >= addr_sp[0xff4a]))
	{
		if (gboy_mode==DMG)
  			render_win(buf);
		else if (gboy_mode==CGB)
			render_win_cgb(buf);
	}
	
	if (addr_sp[0xff40]&2) 
	{
		render_spr();
		if (gboy_mode==DMG)
			for (i=nb_spr-1; i>=0; i--)
				render_spr_dmg(buf, &spr_attr[i]);
		else if (gboy_mode==CGB)
			for (i=nb_spr-1; i>=0; i--)
				render_spr_cgb(buf, &spr_attr[i]);
	}
}

extern long cpu_cur_mode;
extern long hbln_dma_src;
extern long hbln_dma_dst;
extern long hbln_dma;
extern long hdma_on;
long dma_pend=0;
extern long addr_sp_ptrs[16];
void
do_vram_dma(Uint8 val)
{
	long *ptr_addr_ptrs = (long *)&addr_sp_ptrs;
	char *ptr_src;
	char *ptr_dst, *tmp_ptr;
	int val_offs, i, trans_len;

	if (((val&0x80)==0) && (hdma_on))
	{
		addr_sp[0xff55] = 0xff;
		hdma_on=0;
		return;
	}

	trans_len = ((val&0x7f)+1)<<4; // transfer length

	val_offs = *(int *)(addr_sp+0xff51); // offset to pointers
	ptr_src = (char *)(ptr_addr_ptrs[(val_offs&0xf0)>>4]);
	val_offs <<= 8;
	tmp_ptr = (char *)&val_offs;
	tmp_ptr[0] = tmp_ptr[2];
	val_offs = (val_offs&0xfff0); // index to ptr_addr_ptrs
	ptr_src += val_offs;

	ptr_dst = (char *)(ptr_addr_ptrs[8]);
	val_offs = *(int *)(addr_sp+0xff53); // offset to pointers
	val_offs <<= 8;
	tmp_ptr = (char *)&val_offs;
	tmp_ptr[0] = tmp_ptr[2];
	val_offs = (val_offs&0x1ff0); // index to ptr_addr_ptrs
	ptr_dst += val_offs + 0x8000;

	/* General-Purpose DMA */
	if ((val&0x80)==0)
	{
		hdma_on=0; // disable HBlank-Driven DMA
		if (cpu_cur_mode == 1)
			dma_pend = 231 + 16 *(val&0x7f);
		else
			dma_pend = 231 + 8 *(val&0x7f);

		/* Copy stream */
		for (i=0; i<trans_len; i++)
		{
			ptr_dst[i] = ptr_src[i];
			if ((++addr_sp[0xff52])==0)
				++addr_sp[0xff51];
			if ((++addr_sp[0xff54])==0)
				++addr_sp[0xff53];
		}
		addr_sp[0xff55]=0xff;
	}
	/* HBlank-Driven DMA */
	else {
		hdma_on = 1;
		hbln_dma = val&0x7f;
		/* XXX Portable? Any modern architecture with pointers sizes other than 4/8 bytes? */
		hbln_dma_src = (long)ptr_src & (sizeof(char *) == 4 ? (Uint32)(~1) : (Uint64)(~1));
		hbln_dma_dst = (long)ptr_dst & (sizeof(char *) == 4 ? (Uint32)(~1) : (Uint64)(~1));
		addr_sp[0xff55] = val&0x7f;
	}
}

void
do_spr_pal_wr(Uint8 val)
{
	char *ptr_spr_pal = (char *)(*spr_pal);

	ptr_spr_pal+=spr_pal_cur_indx;

	*ptr_spr_pal=val;

	addr_sp[0xff6b] = val;
	if (spr_pal_inc_indx)
	{
		spr_pal_cur_indx++;
		spr_pal_cur_indx &= 0x3f;
		/* Keep bits 8 and 7; increment bits 1-6 */
		addr_sp[0xff6a] = spr_pal_cur_indx|0x80;
	}
}

void
do_back_pal_wr(Uint8 val)
{
	char *ptr_bg_pal = (char *)(*bg_pal);

	ptr_bg_pal+=pal_cur_indx;

	*ptr_bg_pal=val;

	addr_sp[0xff69] = val;
	if (pal_inc_indx)
	{
		pal_cur_indx++;
		pal_cur_indx &=0x3f;
		/* Keep bits 8 and 7; increment bits 1-6 */
		addr_sp[0xff68] = pal_cur_indx|0x80;
	}
}

void
do_spr_pal(Uint8 val)
{
	if (val&0x80)
		spr_pal_inc_indx=1;
	else
		spr_pal_inc_indx=0;

	spr_pal_cur_indx=val&0x3f;
	addr_sp[0xff6a] = val&0xbf;

	/* XXX Hack so byte can be read from background palette with existing code */
	char *ptr_spr_pal = (char *)(*spr_pal);
	ptr_spr_pal+=spr_pal_cur_indx;
	addr_sp[0xff6b] = *ptr_spr_pal; // write byte from palette to register
}

void
do_back_pal(Uint8 val)
{
	if (val&0x80)
		pal_inc_indx=1;
	else
		pal_inc_indx=0;

	pal_cur_indx=val&0x3f;
	addr_sp[0xff68] = val&0xbf;

	/* XXX Hack so byte can be read from background palette with existing code */
	char *ptr_bg_pal = (char *)(*bg_pal);
	ptr_bg_pal+=pal_cur_indx;
	addr_sp[0xff69] = *ptr_bg_pal; // write byte from palette to register
}
