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
extern char gddb_buf[512];

int
addr_is_hex(char *ptr_str, int str_len)
{
	int ret;

	ret=1;
	if (str_len>10 || str_len<3)
		ret=0;
	else {
		if (*ptr_str!='0' || *(ptr_str+1)!='x')
			ret=0;
		ptr_str+=2;
		str_len-=2;
		while (str_len--) {
			switch (*ptr_str++) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					break;
				default:
					ret=0;
					return 0;
			}
			if (ret==0)
				break;
		}
	}

	return ret;
}

void
str_hex_to_num(char *ptr_str, int *ptr_num, int str_len)
{
	int i, m;
	char num[4];
	char buffer[512];
	int *ptr_res;

	for (i=0; i<4; i++)
		num[i]=0;

	while (*ptr_str!='\0')
		ptr_str++;
	ptr_str--;

	i=0;
	m=0;
	while (m<str_len) {
		if (*ptr_str<='9')
			num[i] = ((*ptr_str)-0x30);
		else {
			switch (*ptr_str) {
				case 'a':
					num[i] = 10;
					break;
				case 'b':
					num[i] = 11;
					break;
				case 'c':
					num[i] = 12;
					break;
				case 'd':
					num[i] = 13;
					break;
				case 'e':
					num[i] = 14;
					break;
				case 'f':
					num[i] = 15;
					break;
			}
		}
		ptr_str--;
		if (*ptr_str<='9')
			num[i] |= (((*ptr_str)-0x30)<<4);
		else {
			switch (*ptr_str) {
				case 'a':
					num[i] |= 10<<4;
					break;
				case 'b':
					num[i] |= 11<<4;
					break;
				case 'c':
					num[i] |= 12<<4;
					break;
				case 'd':
					num[i] |= 13<<4;
					break;
				case 'e':
					num[i] |= 14<<4;
					break;
				case 'f':
					num[i] |= 15<<4;
					break;
			}
		}
		i++;
		m+=2;
		ptr_str--;
	}

	ptr_res=(int *)num;
	*ptr_num=*ptr_res;

}

void
hex2ascii32(int *hex_num, int num_bytes, char *buf)
{
	unsigned char cur_byte; // hold the byte being handled
	int num_nibbles = num_bytes * 2; // we work by nibbles
	char *hex_num_local_ptr = (char *)hex_num; // work with byte pointer
	char hexs[]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
							 'c', 'd', 'e', 'f'};

	while (num_nibbles > 0) {
		/* Copy current byte */
		cur_byte = *hex_num_local_ptr; 
		/* If working with low nibble */
		if ((num_nibbles%2) == 0)
			cur_byte &= 0x0f; // extract low nibble
		/* Else if working with high nibble */
		else
			cur_byte &= 0xf0, cur_byte >>= 4; // extract high nibble
		cur_byte=hexs[cur_byte]; // get character
		if ((num_nibbles % 2) == 1) {
			buf[num_nibbles-1] = cur_byte;
			hex_num_local_ptr++;
		}
		else
			buf[num_nibbles-1] = cur_byte;
		num_nibbles--;
	}
}

/*
 * Dumps a contigious block of memory.
 */
void
memdump32(int *mem_arg, char *buf_arg, char num_words, int num_bytes, 
					int num_base)
{
	char reg_num_dump;
	char *buf_arg_back = buf_arg; // save pointer to beginning of string
	int *args_pointer = mem_arg;
	int i;

	for (i=0; i<512; i++)
		gddb_buf[i] = 0;

	for (reg_num_dump = 0; reg_num_dump<num_words; reg_num_dump++) {
		switch (num_base) {
			case 16:
				*buf_arg++ = '0';
				*buf_arg++ = 'x';
				hex2ascii32(args_pointer, num_bytes, buf_arg);
				break;
			case 10:
				//dec2bcd(args_pointer, num_bytes, buf_arg);
				break;
		}
		while (*buf_arg != '\0')
			buf_arg++;
		*buf_arg++ = ' ';
		args_pointer++;
	}
	buf_arg--;
	*buf_arg='\0';
}
