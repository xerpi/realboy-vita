#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

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

AC_PREREQ([2.68])
AC_INIT([RealBoy], [0.1.3])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
AM_PROG_AS(RealBoy, 0.1.3)
AC_CONFIG_SRCDIR([src/gboy_mbc1.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
#AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB(SDL, SDL_Init,, AC_MSG_ERROR(missing SDL library; please install from packet manager))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h termios.h unistd.h SDL/SDL.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([atexit getcwd gettimeofday memset strndup])

AC_ARG_ENABLE([asm],[AS_HELP_STRING([--disable-asm], [disable assembly usage (default is yes on x86_64 archs; no otherwise)])],[enable_asm="$enableval"], [enable_asm=yes])
AC_ARG_ENABLE([cli],[AS_HELP_STRING([--enable-cli], [enable the Command-Line Interface (default is no)])],[enable_cli=yes], [enable_cli="$enablevar"])

AM_CONDITIONAL([USE_CLI], [test "x$enable_cli" = xyes])
AM_CONDITIONAL([USE_X86_64_ASM], [test "x$host_cpu" = xx86_64 -a "x$enable_asm" = xyes])

case "$host_os" in
		openbsd*)
		os_is_obsd=yes
		;;
esac
AM_CONDITIONAL([HOST_IS_OPENBSD], [test "x$os_is_obsd" = xyes])

AC_OUTPUT(Makefile src/Makefile)
