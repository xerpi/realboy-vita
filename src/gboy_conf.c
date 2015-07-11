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
#include "gboy_conf.h"

/*
 * If something goes wrong with Configuration File, just revert
 * to defaults.
 */
static void
apply_conf_defs()
{
	joy_remap('d', 0);
	joy_remap('s', 1);
	joy_remap('\n', 2);
	joy_remap('a', 3);
	set_fps(60);
	vid_scale(1);
	vid_no_fullscreen();
	use_boot_rom=0;
	gboy_mode=0;
	gboy_hw=AUTO;
}

static int
do_conf(char *str, int off)
{
	int i;

	for (i=0; conf_opts[i] != NULL; i++) {
		if (!(strncmp(&str[off], conf_opts[i], strnlen(conf_opts[i], 255))))
			break;
	}

	if (conf_opts[i] == NULL)
		return 0;
	else {
		off += strnlen(conf_opts[i], 255);
		skip_blanks(str, off);
		if (str[off++] != '=')
			return 0;
		skip_blanks(str, off);
		switch (i) {
			case 0:
			case 1:
			case 2:
			case 3:
				if (str[off+1] == '\\') {
					if (str[off] != '\'' || str[off+3] != '\'')
						return 0;
					off++;
					if (!strncmp(&str[off], "\\n", 2))
						joy_remap('\n', i);
					else if (!strncmp(&str[off], "\\t", 2))
						joy_remap('\t', i);
					else if (!strncmp(&str[off], "\\b", 2))
						joy_remap('\b', i);
					else
						return 0;
				}
				else {
					if (str[off] != '\'' || str[off+2] != '\'')
						return 0;
					off++;
					if (joy_remap(str[off], i) == 0)
						return 0;
				}
				break;
			case 4:
				if (!conf_ignore_fps) {
					{
					char num_ascii[3];
					if (str[off] != '\"' || str[off+3] != '\"' || !isdigit(str[off+1]) || !isdigit(str[off+2]))
						return 0;
					off++;
					num_ascii[0] = str[off];
					num_ascii[1] = str[off+1];
					num_ascii[2] = '\0';
					set_fps(atoi(num_ascii));
					}
				}
				break;
			case 5:
				if (!conf_ignore_scale) {
					{
					char num_ascii[2];
					int toint;
					if (str[off] != '\"' || str[off+2] != '\"' || !isdigit(str[off+1]))
						return 0;
					off++;
					num_ascii[0] = str[off];
					num_ascii[1] = '\0';
					toint = atoi(num_ascii);
					if (toint == 1 || toint == 2 ||toint == 3 ||toint == 4)
						vid_scale(toint);
					else
						return 0;
					}
				}
				break;
			case 6:
				if (!conf_ignore_fullscreen) {
					{
					char num_ascii[2];
					if (str[off] != '\"' || str[off+2] != '\"' || !isdigit(str[off+1]))
						return 0;
					off++;
					num_ascii[0] = str[off];
					num_ascii[1] = '\0';
					if (atoi(num_ascii) == 1)
						vid_set_fullscreen();
					else if (atoi(num_ascii) == 0)
						vid_no_fullscreen();
					else
						return 0;
					}
				}
				break;
			case 7:
				if (!conf_ignore_boot) {
					{
					char num_ascii[2];
					int toint;
					if (str[off] != '\"' || str[off+2] != '\"' || !isdigit(str[off+1]))
						return 0;
					off++;
					num_ascii[0] = str[off];
					num_ascii[1] = '\0';
					toint = atoi(num_ascii);
					if (toint == 1)
						use_boot_rom=1;
					else if (toint == 0)
						use_boot_rom=0;
					else
						return 0;
					}
				}
				break;
			case 8:
				if (!conf_ignore_mode) {
					{
					char num_ascii[2];
					int toint;
					if (str[off] != '\"' || str[off+2] != '\"' || !isdigit(str[off+1]))
						return 0;
					off++;
					num_ascii[0] = str[off];
					num_ascii[1] = '\0';
					toint = atoi(num_ascii);
					if (toint == 3)
						gboy_hw=SGB;
					else if (toint == 2)
						gboy_hw=CGB;
					else if (toint == 1)
						gboy_hw=DMG;
					else if (toint == 0)
						gboy_hw=AUTO;
					}
				}
				break;
			case 9:
				;
		}
	}

	return 1;
}

static void
parse_conf(char *conf_str)
{
	int i, valid;

	i = skip_blanks(conf_str, 0);

	while (conf_str[i] != '\0') {
		while (conf_str[i] == '#') {
			i = skip_line(conf_str, i);
			i = skip_blanks(conf_str, i);
		}
		valid = do_conf(conf_str, i);
		if (!valid) {
			printf("\n**********************************\n");
			printf("Invalid configuration option!\n");
			printf("Check your configuration file\n");
			printf("Reverting to defaults...\n");
			printf("**********************************\n\n");
			apply_conf_defs();
			return;
		}
		/* XXX Check rest of line */
		i = skip_line(conf_str, i);
	}
}

static void
apply_conf()
{
	long file_size;
	char *conf_str;

	if ( (file_conf = fopen("RealBoy.conf", "r+")) == NULL)
		perror("fopen()");
	file_size = get_file_size(file_conf);

	if ( (conf_str = malloc(file_size+1)) == NULL)
		perror("malloc()");
	if ( (fread(conf_str, 1, file_size, file_conf)) == 0)
		perror("fread()");

	conf_str[file_size] = '\0';

	parse_conf(conf_str);
}

void
ignore_conf(long ign_opt)
{
	switch (ign_opt) {
		case SCALE:
			conf_ignore_scale = 1;
			break;
		case FULLSCREEN:
			conf_ignore_fullscreen = 1;
			break;
		case FPS:
			conf_ignore_fps = 1;
			break;
		case BOOT:
			conf_ignore_boot = 1;
			break;
		case GB_MODE:
			conf_ignore_mode = 1;
			break;
	}
}

void
init_conf()
{
	/* Get HOME path */
	home_path = get_home_path();

	/* Change working directory to HOME */
	change_cur_dir(home_path);

	/* Search for configuration directory */
	int found_file;
	found_file = search_file_dir(".realboy", home_path);

	/* Create configuration directory if it doesn't exist */
	if (!found_file) {
		printf("\n**********************************\n");
		printf("Configuration Directory not found...\n");
		create_dir(".realboy", 0755);
		printf("Configuration Directory Created %s%s\n", home_path, "/.realboy");
	}
	else
		printf("\n\nFound Configuration Directory %s%s\n", home_path, "/.realboy");
	change_cur_dir(".realboy");

	/* Search for saves directory */
	found_file = search_file_dir("saves", ".");
	/* Create saves directory if it doesn't exist */
	if (!found_file) {
		printf("Saves Directory not found...\n");
		create_dir("saves", 0755);
		printf("Saves Directory Created %s%s\n", home_path, "/.realboy/saves");
	}
	else
		printf("Found Saves Directory %s%s\n", home_path, "/.realboy/saves");

	/* Search for configuration file */
	found_file = search_file_dir("RealBoy.conf", ".");
	/* Create default configuration file if it doesn't exist */
	if (!found_file) {
		printf("Configuration File not found...\n");
		if ( (file_conf = fopen("RealBoy.conf", "w+")) == NULL)
			perror("fopen()");
		printf("Configuration File Created %s%s\n", home_path, "/.realboy/RealBoy.conf");
		if ( (fwrite(default_conf, 1, strnlen(default_conf, 1000), file_conf)) == 0)
			perror("fwrite()");
		printf("**********************************\n");
		apply_conf_defs();
	}
	else {
		printf("Found Configuration File %s%s\n", home_path, "/.realboy/RealBoy.conf");
		printf("Applying Configuration...\n");
		apply_conf();
	}
}
