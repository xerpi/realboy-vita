## Makefile.am -- Process this file with automake to produce Makefile.in
#
# RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
# Copyright (C) 2013 Sergio Andrés Gómez del Real
#
# This program is free software; you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by   
# the Free Software Foundation; either version 2 of the License, or    
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

AUTOMAKE_OPTIONS=no-dependencies
bin_PROGRAMS = realboy
realboy_SOURCES = gboy_conf.c gboy_keys.c gboy_events.c gboy_frame.c gboy_interp.c gboy_ioctrl.c gboy_joy.c gboy_lcd.c gboy_mbc1.c gboy_mbc2.c gboy_mbc3.c gboy_mbc5.c gboy_mbc.c gboy_parse.c gboy_sound.c gboy_video.c gboy_vm.c globals.c gboy_utils.c gboy_file.c main.c SDL_rotozoom.c gddb_main.c gddb_funcs.c gboy.h

if USE_X86_64_ASM
realboy_SOURCES += gboy_x86_64.S
AM_CPPFLAGS = -DUSE_X86_64_ASM
if HOST_IS_OPENBSD
AM_LDFLAGS = -Wl,-non_shared
endif
else
AM_CPPFLAGS =
realboy_SOURCES += gboy_cpu.c
endif
AM_CFLAGS = -Wno-unused-result
