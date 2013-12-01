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
#define SGB_COLOR(c) ((((c)&0x7C00)>>10)|(((c)&0x3E0)<<1)|(((c)&0x1F)<<11))
#define SGB_PACKSIZE 16

struct mask_shift {
  unsigned char mask;
  unsigned char shift;
};
struct mask_shift tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

Uint8 sgb_pack_buf[112];
Uint8 sgb_tiles[256*32];
Uint8 sgb_map[32*32];
Uint8 sgb_att[32*32];
long sgb_cur_cmd=-1;
long sgb_cur_pack=0;
long sgb_flag=0;
long sgb_mask=0;
long sgb_cur_bit=0;
long sgb_multi_player=0;
long sgb_four_players=0;
long sgb_next_ctrlr=0;
long sgb_cur_bit_shf=0;
long sgb_state=IDLE;
Uint32 sgb_scpal[512][4];
Uint32 sgb_border_pal[64];
Uint8 sgb_ATF[45][90];

/* SGB Border */

void sgb_draw_one_tile(Uint32 *buf,int x,int y,Uint16 no_tile,Uint8 att,Uint16 pitch) {
  int sx,sy;
  char bit0,bit1,bit2,bit3;
  int c;
  char xflip=att&0x40;
  Uint32 *b=&buf[x+y*(pitch>>2)];
  pitch=pitch>>2;
  /* FIXME: comme ca dkl2 et conker ca marche mais dragon quest nan :(*/
 
  if (sgb_flag) {
    if (no_tile>=128) no_tile=((64+no_tile)%128)+128;
    else  no_tile=(64+no_tile)%128;
  }
  
  no_tile=no_tile|((att&0x03)<<6);

  if (att&0x80) {    // yflip
    for(sy=7;sy>=0;sy--,b+=pitch) {
      for(sx=0;sx<8;sx++) {
	int wbit;
	if (!xflip) wbit=sx;
	else wbit=7-sx;
	bit0=(sgb_tiles[no_tile*32+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=(sgb_tiles[no_tile*32+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit2=(sgb_tiles[no_tile*32+16+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit3=(sgb_tiles[no_tile*32+16+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit3<<3)|(bit2<<2)|(bit1<<1)|bit0;
	b[sx]=sgb_border_pal[c|((att&0x0c)<<2)];
      }
    }
  } else {
    for(sy=0;sy<8;sy++,b+=pitch) {
      for(sx=0;sx<8;sx++) {
	int wbit;
	if (!xflip) wbit=sx;
	else wbit=7-sx;
	bit0=(sgb_tiles[no_tile*32+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=(sgb_tiles[no_tile*32+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit2=(sgb_tiles[no_tile*32+16+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit3=(sgb_tiles[no_tile*32+16+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit3<<3)|(bit2<<2)|(bit1<<1)|bit0;
	b[sx]=sgb_border_pal[c|((att&0x0c)<<2)];
      }
    }
  }
}

void sgb_draw_tiles(void) {
  int i,j;
  Uint32 *buf;
  Uint16 pitch;

  buf=(Uint32 *)sgb_buf->pixels;
  pitch=sgb_buf->pitch;

  for(j=0;j<28;j++) 
    for(i=0;i<32;i++) 
      sgb_draw_one_tile(buf,i*8,j*8,sgb_map[i+j*32],sgb_att[i+j*32],pitch);

  vid_sgb_mask();
}

/* Area Designation Mode Functions */

void set_pal_Hline(int line,int pal) {
  int i;
  for(i=0;i<20;i++)
    sgb_pal_map[i][line]=pal;
}

void set_pal_Vline(int line,int pal) {
  int i;
  for(i=0;i<18;i++)
    sgb_pal_map[line][i]=pal; 
}

void set_pal_Hline_range(int l1,int l2,int pal) {
  int i,j;
  for(j=l1;j<=l2;j++)
    for(i=0;i<20;i++)
      sgb_pal_map[i][j]=pal;
}

void set_pal_Vline_range(int l1,int l2,int pal) {
  int i,j;
  for(i=l1;i<=l2;i++)
    for(j=0;j<18;j++)    
      sgb_pal_map[i][j]=pal;
}

void set_pal_inside_block(int x1,int y1,int x2,int y2,int pal) {
  int x,y;
  if (x1<0) x1=0;
  if (x2>=20) x2=20;
  if (y1<0) y1=0;
  if (y2>=18) y2=18;
  for(x=x1+1;x<x2;x++)
    for(y=y1+1;y<y2;y++)
      sgb_pal_map[x][y]=pal;
}

void set_pal_outside_block(int x1,int y1,int x2,int y2,int pal) {
  if (y1>0)
    set_pal_Hline_range(0,y1-1,pal);
  if (y2<17)
    set_pal_Hline_range(y2+1,17,pal);
  if (x1>0)
    set_pal_Vline_range(0,x1-1,pal);
  if (x2<19)
    set_pal_Vline_range(x2+1,19,pal);
}

void set_pal_on_block(int x1,int y1,int x2,int y2,int pal) {
  int x,y;
  if (x1<0) x1=0;
  if (x2>=20) x2=19;
  for(x=x1;x<=x2;x++) {
    if (y1>=0)
      sgb_pal_map[x][y1]=pal;
    if (y2<18)
      sgb_pal_map[x][y2]=pal;
  }

  if (y1<0) y1=0;
  if (y2>=18) y2=17;
  for(y=y1;y<=y2;y++) {
    if (x1>=0)
      sgb_pal_map[x1][y]=pal;
    if (x2<20)
      sgb_pal_map[x2][y]=pal;
  }
}

static void
set_ATF_nf(int nf,int mode) {
  int i,j,n;
  Uint8 *t=sgb_ATF[nf];
  /*printf("Set data from ATF \n");
    printf("Num file %d\n",nf);
    printf("Mode %d\n",mode);*/
  
  for(j=0;j<18;j++) {
    i=0;
    for(n=0;n<5;n++,t++) {
      sgb_pal_map[i++][j]=(*t)>>6;
      sgb_pal_map[i++][j]=((*t)&0x30)>>4;
      sgb_pal_map[i++][j]=((*t)&0x0c)>>2;
      sgb_pal_map[i++][j]=((*t)&0x03);
    }
  }
//  if (mode) sgb_mask=0;
}

static void
sgb_att_blk()
{
	static long sgb_num_pack=-1;
  static Sint8 nb_dataset;
  static Uint8 dataset[6];
  static Uint8 ds_i; // dataset indice  
  Uint8 p_i;   // packet indice

  if (sgb_num_pack==-1) {  // first call
    //printf("Block mode\n");
    sgb_num_pack=sgb_pack_buf[0]&0x07;
    nb_dataset=sgb_pack_buf[1]&0x1f;
    //printf("nb dataset %02x\n",nb_dataset);
    p_i=2;
    ds_i=0;
  } else p_i=0;

  while(nb_dataset>0 && p_i<SGB_PACKSIZE) {
    dataset[ds_i++]=sgb_pack_buf[p_i++];
    if (ds_i==6) { // on traite le dataset
      Sint8 SH,SV,EH,EV;
      Uint8 px,py,pz;
      SH=dataset[2]&0x1f;
      SV=dataset[3]&0x1f;
      EH=dataset[4]&0x1f;
      EV=dataset[5]&0x1f;
      px=(dataset[1]>>4)&0x03;
      py=(dataset[1]>>2)&0x03;
      pz=(dataset[1]&0x03);
    
      nb_dataset--;
      
      //printf("type %d p %d %d %d line  %d %d %d %d\n",dataset[0]&0x07,px,py,pz,SH,SV,EH,EV);
      ds_i=0;
      switch(dataset[0]&0x07) {
      case 0x00:break;
      case 0x01:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_on_block(SH,SV,EH,EV,pz);
	break;
      case 0x02:
	set_pal_on_block(SH,SV,EH,EV,py);
	break;
      case 0x03:
	set_pal_inside_block(SH,SV,EH,EV,py);
	set_pal_on_block(SH,SV,EH,EV,py);
	break;
      case 0x04:
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;	
      case 0x05:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;
      case 0x06:
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;
      case 0x07:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_on_block(SH,SV,EH,EV,py);
	set_pal_outside_block(SH,SV,EH,EV,px);	  
	break;
      }
    }
  }
  
  sgb_num_pack--;
  if (sgb_num_pack==0) sgb_num_pack=-1;

}
static void
sgb_att_lin()
{
	static long sgb_num_pack=-1;
  int i;
  static Sint16 nb_dataset;

  if (sgb_num_pack==-1) {  // first call
    sgb_num_pack=sgb_pack_buf[0]&0x07;
    //printf("%d\n",sgb_num_pack);
    //printf("Line mode\n");
    nb_dataset=sgb_pack_buf[1];
    //printf("nb dataset %d\n",nb_dataset);
    i=2;
  } else i=0;

  while(nb_dataset>0 && i<SGB_PACKSIZE) {
    nb_dataset--;
    /*printf("mode %d\n",(sgb_pack_buf[i]&0x40));
    printf("line %d\n",sgb_pack_buf[i]&0x1f);
    printf("pal %d\n",(sgb_pack_buf[i]&0x60)>>5);*/
    if ((sgb_pack_buf[i]&0x40))  // mode vertical
      set_pal_Vline(sgb_pack_buf[i]&0x1f,(sgb_pack_buf[i]>>5)&0x03);
    //set_pal_Vline_range(0,sgb_pack_buf[i]&0x1f,(sgb_pack_buf[i]>>5)&0x03);
    else  // mode horizontal
      set_pal_Hline(sgb_pack_buf[i]&0x1f,(sgb_pack_buf[i]>>5)&0x03);
    //set_pal_Hline_range(0,sgb_pack_buf[i]&0x1f,(sgb_pack_buf[i]>>5)&0x03);

    i++;
  }

  sgb_num_pack--;
  if (sgb_num_pack==0) sgb_num_pack=-1;

}
static void
sgb_att_div()
{
  int line=sgb_pack_buf[2]&0x1f;
  /*printf("Divide mode\n");
    printf(((sgb_pack_buf[1]&0x40)?"Vertical\n":"Horizontal\n"));
    printf("Line %d\n",sgb_pack_buf[2]);
    printf("Color %d %d %d \n",(sgb_pack_buf[1]&0x30)>>4,(sgb_pack_buf[1]&0x0c)>>2,sgb_pack_buf[1]&0x03);*/
  
  if (!(sgb_pack_buf[1]&0x40)) { 
    set_pal_Vline(line,(sgb_pack_buf[1]&0x30)>>4);
    set_pal_Vline_range(0,line-1,(sgb_pack_buf[1]>>2)&0x03);
    set_pal_Vline_range(line+1,19,(sgb_pack_buf[1]/*>>2*/)&0x03);
  } else { 
    set_pal_Hline(line,(sgb_pack_buf[1]&0x30)>>4);
    set_pal_Hline_range(0,line-1,(sgb_pack_buf[1]>>2)&0x03);
    set_pal_Hline_range(line+1,17,(sgb_pack_buf[1]/*>>2*/)&0x03);
  }

}
static void
sgb_att_chr()
{
	static long sgb_num_pck=-1;
  static Uint8 mode;
  static Uint16 nb_dataset;
  static Uint8 I,J;
  static int i;

  if (sgb_num_pck==-1) {  // first call
    sgb_num_pck=sgb_pack_buf[0]&0x07;
    /*printf("1chr mode\n");
      printf("Nb packet %d\n",sgb_pack_buf[0]&0x07);
      printf("X %d Y %d\n",sgb_pack_buf[1]&0x1f,sgb_pack_buf[2]&0x1f);
      printf("Nb dataset %d\n",sgb_pack_buf[3]);
      printf("MSB %d\n",sgb_pack_buf[4]);
      printf("Writing style %d\n",sgb_pack_buf[5]);
      printf("Data ....\n");*/
    
    I=(sgb_pack_buf[1]&0x1f); J=(sgb_pack_buf[2]&0x1f);
    nb_dataset=((sgb_pack_buf[4]&0x01)<<8)|sgb_pack_buf[3];
    if (nb_dataset>360) nb_dataset=360;
    mode=sgb_pack_buf[5]&0x01;
    i=6;
  } else i=0;
  
  while(i<SGB_PACKSIZE) {
    int t,p;
    for(t=3;t>=0;t--) {
      //p=(sgb_pack_buf[i]&(0x03<<(t*2)))>>(t*2);
      p=(sgb_pack_buf[i]>>(t*2))&0x03;
      sgb_pal_map[I][J]=p;
      
      if (!mode) {
	I=(I+1)%20;
	if (!I) J++;
      } else {
	J=(J+1)%18;
	if (!J) I++;
      }
    }
    i++;
  }
  
  sgb_num_pck--;
  //printf("packet num %d\n",sgb_num_pack);
  if (sgb_num_pck==0) sgb_num_pck=-1;

}
static void
sgb_dummy()
{

}
static void
sgb_chr_trn()
{
  //char type=sgb_pack_buf[1]>>2;
  Uint8 range=sgb_pack_buf[1]&0x01;
  Uint8 *src,*dst;

  //  printf("Pack %02x\n",sgb_pack_buf[1]);
  src=&addr_sp[0x8800];
  dst=(!range)?(&sgb_tiles[0]):(&sgb_tiles[0x80*32]);
  memcpy(dst,src,0x80*32);
  
}
static void
sgb_pic_trn()
{
  int i;
	Uint32 cur_col, col16, r, g, b;

  //printf("%02x\n",LCDCCONT);

  for(i=0;i<32*32;i++) {
    /* FIXME: dkl2 et conker => 0x1000 */
    if (sgb_flag) {
      sgb_map[i]=addr_sp[0x9000+i*2];
      sgb_att[i]=addr_sp[0x9000+i*2+1];
    } else {
      sgb_map[i]=addr_sp[0x8800+i*2];
      sgb_att[i]=addr_sp[0x8800+i*2+1];
    }
  }
  
  /* FIXME: dkl2 et conker => 0x800 */
  /* TODO: Fix sgb border color 0 */
  if (sgb_flag) 
    for(i=0;i<64;i++) {
      		col16=SGB_COLOR((addr_sp[0x8800+i*2+1]<<8)|addr_sp[0x8800+i*2]);
			r = (col16&0xf800)>>11;
			g = (col16 & 0x7e0)>>5;
			b = col16&0x1f;
			r = r * 255 / 31;
			g = g * 255 / 63;
			b = b * 255 / 31;
			cur_col = (r << 16) | (g<<8) | (b);
			
      sgb_border_pal[i]=cur_col;
	}
  else 
    for(i=0;i<64;i++) {
      		col16=SGB_COLOR((addr_sp[0x9000+i*2+1]<<8)|addr_sp[0x9000+i*2]);
			r = (col16&0xf800)>>11;
			g = (col16 & 0x7e0)>>5;
			b = col16&0x1f;
			r = r * 255 / 31;
			g = g * 255 / 63;
			b = b * 255 / 31;
			cur_col = (r << 16) | (g<<8) | (b);
      sgb_border_pal[i]=cur_col;
	}
  
			r = (0xffff&0xf800)>>11;
			g = (0xffff & 0x7e0)>>5;
			b = 0xffff&0x1f;
			r = r * 255 / 31;
			g = g * 255 / 63;
			b = b * 255 / 31;
			cur_col = (r << 16) | (g<<8) | (b);
      sgb_border_pal[0]=cur_col;

  sgb_draw_tiles();
}
static void
sgb_att_atf()
{
  int i;
  Uint8 *t=&addr_sp[0x8800];
  
  //printf("Set ATF \n");
  for(i=0;i<45;i++,t+=90)
    memcpy(sgb_ATF[i],t,90);  
  set_ATF_nf(0,0);
}
static void
sgb_dat_atf()
{
  set_ATF_nf(sgb_pack_buf[1]&0x3f,sgb_pack_buf[1]&0x40);
}
static void
sgb_win_msk()
{
  switch(sgb_pack_buf[1]&0x03) {
  case 0x00:sgb_mask=0;break;
  case 0x01:  // i dint know what it do
  case 0x02:
    /*for(i=0;i<4;i++)
      memset(sgb_pal[i],0,sizeof(Uint16)*4);
      break;*/
  case 0x03:
    /*for(i=0;i<4;i++)
      memset(sgb_pal[i],0xffff,sizeof(Uint16)*4);
      break;*/
    sgb_mask=1;
    break;   
  }

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
	Uint32 cur_col, col16, r, g, b;
	Uint8 *t=&addr_sp[0x8800];
 
	for(i=0;i<512;i++) {
		for(j=0;j<4;j++,t+=2) {
		//	cur_col = (((*t&0x1f)<<3)<<16) | ((((*t>>5)&0x1f)<<3)<<8) | (((*t>>10)&0x1f)<<3);
      		col16=SGB_COLOR((*(t+1)<<8)|(*t));
			r = (col16&0xf800)>>11;
			g = (col16 & 0x7e0)>>5;
			b = col16&0x1f;
			r = r * 255 / 31;
			g = g * 255 / 63;
			b = b * 255 / 31;
			cur_col = (r << 16) | (g<<8) | (b);
			sgb_scpal[i][j]=cur_col;
		}
	}

}
static void
sgb_set_pal_ind()
{
	memcpy(pal_sgb[0],sgb_scpal[((sgb_pack_buf[2]&0x01)<<8)|sgb_pack_buf[1]],2*8);
	memcpy(pal_sgb[1],sgb_scpal[((sgb_pack_buf[4]&0x01)<<8)|sgb_pack_buf[3]],2*8);
	memcpy(pal_sgb[2],sgb_scpal[((sgb_pack_buf[6]&0x01)<<8)|sgb_pack_buf[5]],2*8);
	memcpy(pal_sgb[3],sgb_scpal[((sgb_pack_buf[8]&0x01)<<8)|sgb_pack_buf[7]],2*8);

	if (sgb_pack_buf[9])
		set_ATF_nf(sgb_pack_buf[9]&0x3f, 1);
}

static void
sgb_set_pal1()
{
	Uint32 i, cur_col, num, col16, r, g, b;
	Uint8 pal1=0, pal2=1;

	for(i=0;i<4;i++) {
		col16 = SGB_COLOR((sgb_pack_buf[2]<<8)|sgb_pack_buf[1]);
		r = (col16&0xf800)>>11;
		g = (col16 & 0x7e0)>>5;
		b = col16&0x1f;
		r = r * 255 / 31;
		g = g * 255 / 63;
		b = b * 255 / 31;
		cur_col = (r << 16) | (g<<8) | (b);
		pal_sgb[i][0]=cur_col;
	}
	col16 = SGB_COLOR((sgb_pack_buf[4]<<8)|sgb_pack_buf[3]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][1]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[6]<<8)|sgb_pack_buf[5]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[8]<<8)|sgb_pack_buf[7]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][3]=cur_col;
	
	col16 = SGB_COLOR((sgb_pack_buf[10]<<8)|sgb_pack_buf[9]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][1]=cur_col;


	col16 = SGB_COLOR((sgb_pack_buf[12]<<8)|sgb_pack_buf[11]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[14]<<8)|sgb_pack_buf[13]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][3]=cur_col;
}
static void
sgb_set_pal2()
{
	Uint32 i, cur_col, num, col16, r, g, b;
	Uint8 pal1=2, pal2=3;

	for(i=0;i<4;i++) {
		col16 = SGB_COLOR((sgb_pack_buf[2]<<8)|sgb_pack_buf[1]);
		r = (col16&0xf800)>>11;
		g = (col16 & 0x7e0)>>5;
		b = col16&0x1f;
		r = r * 255 / 31;
		g = g * 255 / 63;
		b = b * 255 / 31;
		cur_col = (r << 16) | (g<<8) | (b);
		pal_sgb[i][0]=cur_col;
	}
	col16 = SGB_COLOR((sgb_pack_buf[4]<<8)|sgb_pack_buf[3]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][1]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[6]<<8)|sgb_pack_buf[5]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[8]<<8)|sgb_pack_buf[7]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][3]=cur_col;
	
	col16 = SGB_COLOR((sgb_pack_buf[10]<<8)|sgb_pack_buf[9]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][1]=cur_col;


	col16 = SGB_COLOR((sgb_pack_buf[12]<<8)|sgb_pack_buf[11]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[14]<<8)|sgb_pack_buf[13]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][3]=cur_col;
}
static void
sgb_set_pal3()
{
	Uint32 i, cur_col, num, col16, r, g, b;
	Uint8 pal1=0, pal2=3;

	for(i=0;i<4;i++) {
		col16 = SGB_COLOR((sgb_pack_buf[2]<<8)|sgb_pack_buf[1]);
		r = (col16&0xf800)>>11;
		g = (col16 & 0x7e0)>>5;
		b = col16&0x1f;
		r = r * 255 / 31;
		g = g * 255 / 63;
		b = b * 255 / 31;
		cur_col = (r << 16) | (g<<8) | (b);
		pal_sgb[i][0]=cur_col;
	}
	col16 = SGB_COLOR((sgb_pack_buf[4]<<8)|sgb_pack_buf[3]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][1]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[6]<<8)|sgb_pack_buf[5]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[8]<<8)|sgb_pack_buf[7]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][3]=cur_col;
	
	col16 = SGB_COLOR((sgb_pack_buf[10]<<8)|sgb_pack_buf[9]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][1]=cur_col;


	col16 = SGB_COLOR((sgb_pack_buf[12]<<8)|sgb_pack_buf[11]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[14]<<8)|sgb_pack_buf[13]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][3]=cur_col;
}
static void
sgb_set_pal4()
{
	Uint32 i, cur_col, num, col16, r, g, b;
	Uint8 pal1=1, pal2=2;

	for(i=0;i<4;i++) {
		col16 = SGB_COLOR((sgb_pack_buf[2]<<8)|sgb_pack_buf[1]);
		r = (col16&0xf800)>>11;
		g = (col16 & 0x7e0)>>5;
		b = col16&0x1f;
		r = r * 255 / 31;
		g = g * 255 / 63;
		b = b * 255 / 31;
		cur_col = (r << 16) | (g<<8) | (b);
		pal_sgb[i][0]=cur_col;
	}
	col16 = SGB_COLOR((sgb_pack_buf[4]<<8)|sgb_pack_buf[3]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][1]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[6]<<8)|sgb_pack_buf[5]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[8]<<8)|sgb_pack_buf[7]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal1][3]=cur_col;
	
	col16 = SGB_COLOR((sgb_pack_buf[10]<<8)|sgb_pack_buf[9]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][1]=cur_col;


	col16 = SGB_COLOR((sgb_pack_buf[12]<<8)|sgb_pack_buf[11]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][2]=cur_col;

	col16 = SGB_COLOR((sgb_pack_buf[14]<<8)|sgb_pack_buf[13]);
	r = (col16&0xf800)>>11;
	g = (col16 & 0x7e0)>>5;
	b = col16&0x1f;
	r = r * 255 / 31;
	g = g * 255 / 63;
	b = b * 255 / 31;
	cur_col = (r << 16) | (g<<8) | (b);
	pal_sgb[pal2][3]=cur_col;
}

void (*sgb_cmds[0x32])() = { sgb_set_pal1, sgb_set_pal2, sgb_set_pal3, sgb_set_pal4, sgb_att_blk, sgb_att_lin, sgb_att_div, sgb_att_chr, sgb_dummy, sgb_dummy, sgb_set_pal_ind, sgb_set_pal_scp, sgb_dummy, sgb_dummy, sgb_dummy, sgb_dummy, sgb_dummy, sgb_mul_req, sgb_dummy, sgb_chr_trn, sgb_pic_trn, sgb_att_atf, sgb_dat_atf, sgb_win_msk, sgb_dummy, sgb_dummy };

static void
sgb_reset()
{
	sgb_cur_bit=0;
	sgb_state=RESET;
	if (sgb_cur_cmd == -1) {
		sgb_cur_bit_shf=0;
		memset(sgb_pack_buf, 0, SGB_PACK_SIZE*SGB_MAX_PACK);
	}
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
				if (sgb_cur_cmd != -1) {
					(sgb_cmds[sgb_cur_cmd])();
					sgb_cur_pack++;
				}
				else {
					sgb_cur_cmd = sgb_pack_buf[0]>>3;
					(sgb_cmds[sgb_pack_buf[0]>>3])();
					sgb_state=IDLE; // Shut down
					sgb_cur_pack++;
				}
				if (((sgb_cur_bit_shf) / 128) == (sgb_pack_buf[0]&7)) {
					sgb_cur_cmd = -1;
					sgb_cur_pack=0;
				}
				sgb_cur_bit_shf++;
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
