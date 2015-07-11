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

/* External symbols */
extern Uint32 gboy_hw; // Game Boy/Color Game Boy hardware
extern char *file_path;
extern int use_boot_rom;
extern int gbddb;
extern Uint32 scale;
extern Uint32 fullscreen;
extern int frames_per_second;
extern int start_vm();
extern void vid_scale(int);
extern void set_fps(int);
extern void ignore_conf(long);
extern void vid_toggle_fullscreen();
extern void init_conf();

#ifndef VITA
/* Locally-global variables*/
struct option options[] = {
	{ "video-1x", no_argument, 0, '1' },
	{ "video-2x", no_argument, 0, '2' },
	{ "video-3x", no_argument, 0, '3' },
	{ "video-4x", no_argument, 0, '4' },
	{ "debug", no_argument, 0, 'd' },
	{ "frame-rate", required_argument, 0, 'r' },
	{ "fullscreen", no_argument, 0, 'f' },
	{ "with-boot", no_argument, 0, 'b' },
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "DMG", no_argument, 0, 'D' },
	{ "CGB", no_argument, 0, 'C' },
	{ "SGB", no_argument, 0, 'S' },
	{ NULL, no_argument, 0, 0 }
};
#endif
