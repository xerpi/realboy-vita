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
/* Imported/External functions for commands. Defined in gboy_cmd.c */
extern int with_boot;
extern Uint32 scale;
extern Uint32 fullscreen;
extern int frames_per_second;
extern void gboy_ddb(int, char **);
extern void gboy_play(int, char **);
extern void gboy_load(int, char **);
extern void gboy_help(int, char **);
extern void gboy_fps(int, char **);
extern void gboy_mode(int, char **);

/* Pointers to command functions */
static void (*gboy_cmds_funcs[NUM_CMDS])(int, char **) = { gboy_help, gboy_load, gboy_play, gboy_ddb, gboy_fps, gboy_mode };
static char welcome_message[] = "\n==================================\nWelcome to RealBoy: A Complete, Fast, Yet Accurate, Emulator for the Nintendo® Game Boy®/Game Boy Color®\nType help\n";
static const char * const gboy_cmds[] = { "help", "load", "play", "debug", "fps", "mode" };

struct option options[] = {
	{ "video-2x", no_argument, 0, '2' },
	{ "video-3x", no_argument, 0, '3' },
	{ "video-4x", no_argument, 0, '4' },
	{ "debug", no_argument, 0, 'd' },
	{ "frame-rate", required_argument, 0, 'r' },
	{ "fullscreen", no_argument, 0, 'f' },
	{ "with-boot", no_argument, 0, 'b' },
	{ "help", no_argument, 0, 'h' },
	{ NULL, no_argument, 0, 0 }
};

/*
 * Call the interpreter until a file is loaded and the 'play' command executed
 */
static void
gboy_init_interp()
{
	gbplay=0;

	printf("%s", welcome_message);
	while (gbplay == 0)
		gboy_interp("gboy> ", NUM_CMDS, gboy_cmds, gboy_cmds_funcs, NULL);
}

static void
usage(char *cmd)
{
  printf("%s [option ...] file\n", cmd);
  printf("\
\n\
Options:\n\
  -2, --video-2x               2x scale\n\
  -3, --video-3x               3x scale\n\
  -4, --video-4x               4x scale\n\
  -r, --frame-rate=RATE        Set frame rate (range [10, 60])\n\
  -f, --fullscreen             Fullscreen\n\
  -b, --with-boot              Execute boot ROM\n\
  -h, --help                   Print this help\n\
  -d, --debug                  Enable GDDB debugger\n\
");
}

/*
 * Main function.
 * Loops through the command-line interpreter until ready to play.
 * XXX implement argv
 */
int
main(int argc, char *argv[])
{
#ifdef USE_CLI
	while (1) {
		gboy_init_interp(); // call the command-line interface
		while (1) {
			/*
			 * start_vm() returns when the file opened doesn't represent
			 * a valid Game Boy ROM, or when the user chooses to change
			 * the ROM.
			 */
			if ((start_vm(rom_fd))==-1)
				printf("File not a gb binary\n\n");
			close(rom_fd);
			rom_fd=0;
			break;
		}
	}
#else
	int op;
	int arg;

	if (argc==1) {
		usage(argv[0]);
		return 0;
	}

	do {
		op = getopt_long(argc, argv, "r:234fhdb", options, NULL);
		switch (op) {
			case '2':
					scale=2;
					break;
			case '3':
					scale=3;
					break;
			case '4':
					scale=4;
					break;
			case 'r':
					arg = atoi(optarg);
					if (arg >= 10 && arg <= 60)
						frames_per_second = arg;
					else
						printf("Bad argument for frame rate; defaulting to 60fps\n");
					break;
			case 'f':
					fullscreen=1;
					break;
			case 'b':
				with_boot=1;
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			case 'd':
				gbddb=1;
				break;
			default:
				break;

		}
	} while (op != -1);

	if (optind < argc) {
		if ( (rom_fd=open(argv[optind], O_RDONLY)) == -1)
			perror(argv[optind]);
		if ((start_vm(rom_fd))==-1)
			printf("File not a gb binary\n\n");
	}
#endif


	/* Execution never gets here; the program exits through a callback function */
	return 0;
}
