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
#include "gboy_events_vita.h"

unsigned char turbo = 0;
long key_bitmap = 0;
static SceCtrlData pad;
static SceCtrlData old_pad;

#define CHANGE_GAME_MASK (SCE_CTRL_TRIANGLE | SCE_CTRL_LTRIGGER)
#define FULLSCREEN_MASK  (SCE_CTRL_RTRIGGER)
#define TURBO_MASK       (SCE_CTRL_LTRIGGER)
#define JOY_THRESHOLD    110

static const struct {
	int vita, gb;
} key_map[] = {
	{SCE_CTRL_CROSS,  D_MASK},
	{SCE_CTRL_CIRCLE, S_MASK},
	{SCE_CTRL_SELECT, A_MASK},
	{SCE_CTRL_START,  RET_MASK},
	{SCE_CTRL_UP,     UP_MASK},
	{SCE_CTRL_DOWN,   DOWN_MASK},
	{SCE_CTRL_LEFT,   LEFT_MASK},
	{SCE_CTRL_RIGHT,  RIGHT_MASK},
};

int joy_remap(char key_ascii, int key_remap)
{
	return 0;
}

// Returning 1 means change game
long proc_evts()
{
	int i;
	unsigned int pressed;
	sceCtrlPeekBufferPositive(0, &pad, 1);

	pressed = pad.buttons & ~old_pad.buttons;

	if ((pad.buttons & CHANGE_GAME_MASK) == CHANGE_GAME_MASK) {
		return 1;
	} else if ((pressed & FULLSCREEN_MASK) == FULLSCREEN_MASK) {
		vid_toggle_fullscreen();
	} else if ((pressed & TURBO_MASK) == TURBO_MASK) {

		turbo ^= 1;

		if (turbo)
			frame_speedup();

		else
			frame_speeddown();
	}

	key_bitmap = 0;

	for (i = 0; i < sizeof(key_map)/sizeof(*key_map); i++) {
		if (pad.buttons & key_map[i].vita) {
			key_bitmap |= key_map[i].gb;
		}
	}

	if (pad.lx < (128 - JOY_THRESHOLD)) key_bitmap |= LEFT_MASK;
	else if (pad.lx > (128 + JOY_THRESHOLD)) key_bitmap |= RIGHT_MASK;

	if (pad.ly < (128 - JOY_THRESHOLD)) key_bitmap |= UP_MASK;
	else if (pad.ly > (128 + JOY_THRESHOLD)) key_bitmap |= DOWN_MASK;

	old_pad = pad;
	return 0;
}
