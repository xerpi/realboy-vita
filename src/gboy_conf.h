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
/* Globally local variables */
long conf_ignore_scale = 0;
long conf_ignore_fullscreen = 0;
long conf_ignore_fps = 0;
long conf_ignore_boot = 0;
long conf_ignore_mode = 0;
static const char default_conf[] = "# RealBoy Configuration File.\n\n# Key mappings\nA='d'\nB='s'\nStart='\\n'\nSelect='a'\n\n# Frame rate setting. Values are from 10 to 60 inclusive.\nframe_rate=\"60\"\n\n# Video Scale: 1x, 2x, 3x, 4x.\nvideo_scale=\"1\"\n\n# Use fullscreen mode. 0=false, any other value means true\nvideo_fullscreen=\"0\"\n\n# Use boot ROM. 0=false, any other value means true\nboot_rom=\"0\"\n\n# Game Boy type. 0=Auto, 1=Force DMG, 2=Force CGB, 3=Force SGB\ngboy_type=\"0\"";
static const char *conf_opts[] = { "A", "B", "Start", "Select", "frame_rate", "video_scale", "video_fullscreen", "boot_rom", "gboy_type", NULL };
static FILE *file_conf;

/* External symbols */
extern Uint32 gboy_hw; // Game Boy/Color Game Boy hardware
extern Uint32 gboy_mode;
extern int use_boot_rom;
extern char *home_path;
extern void vid_scale(Uint32);
extern void set_fps(int);
extern int joy_remap(char, int);
extern long get_file_size(FILE *);
extern void change_cur_dir(char *);
extern int search_file_dir(char *, char *);
extern char *get_home_path();
extern void create_dir(char *, Uint32);
extern int skip_line(char *, int);
extern int skip_blanks(char *, int);
extern void vid_no_fullscreen();
extern void vid_set_fullscreen();
