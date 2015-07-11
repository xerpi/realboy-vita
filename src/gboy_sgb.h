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
/*  gngb, a game boy color emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#define SGB_PACK_SIZE 16
#define SGB_MAX_PACK 7
#define IDLE 0
#define RESET 1
#define SENDING 2
#define SGB_COLOR(c) ((((c)&0x7C00)>>10)|(((c)&0x3E0)<<1)|(((c)&0x1F)<<11))
#define SGB_PACKSIZE 16

extern Uint32 pal_sgb[4][4]; // 32-bit palette for Super Game Boy
extern Uint8 sgb_pal_map[20][18];
#ifdef VITA
extern Surface *sgb_buf;
#else
extern SDL_Surface *sgb_buf;
#endif

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
long sgb_next_ctrlr=0x0f;
long sgb_reading_ctrlr=0;
long sgb_cur_bit_shf=0;
long sgb_state=IDLE;
Uint32 sgb_scpal[512][4];
Uint32 sgb_border_pal[64];
Uint8 sgb_ATF[45][90];
