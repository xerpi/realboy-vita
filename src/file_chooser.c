#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>
#include <psp2/types.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>
#include "font.h"
#include "file_chooser.h"

#define GAME_EXIT_COMBO (SCE_CTRL_SELECT)

#define SCREEN_W 960
#define SCREEN_H 544
#define LIST_MAX_ONSCREEN ((SCREEN_H-40)/20)

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define WHITE RGBA8(255, 255, 255, 255)
#define GREEN RGBA8(0,   255, 0  , 255)

typedef struct file_list_entry {
	char name[PATH_MAX];
	int is_dir;
	int supported;
	struct file_list_entry *next;
} file_list_entry;

typedef struct file_list {
	file_list_entry *head;
	int length;
	int scroll;
} file_list;


static void file_list_add_entry(file_list *list, file_list_entry *entry)
{
	if (list->head == NULL) {
		list->head = entry;
		entry->next = NULL;
	} else {
		file_list_entry *tmp = list->head;
		list->head = entry;
		entry->next = tmp;
	}
	list->length++;
}

static void file_list_empty(file_list *list)
{
	file_list_entry *p = list->head;
	file_list_entry *q;
	while (p) {
		q = p->next;
		free(p);
		p = q;
	}
	memset(list, 0, sizeof(*list));
}

static int file_supported(const char *filename, const char *supported_ext[])
{
	int i;
	const char *ext = strrchr(filename, '.');
	if (ext) {
		i = 0;
		while (supported_ext[i]) {
			if (strcmp(ext + 1, supported_ext[i]) == 0) {
				return 1;
			}
			i++;
		}
	}
	return 0;
}

static int file_list_build(const char *path, file_list *list, const char *supported_ext[])
{
	SceUID dir;
	SceIoDirent dirent;

	dir = sceIoDopen(path);
	if (dir < 0) {
		return 0;
	}

	memset(&dirent, 0, sizeof(dirent));
	memset(list, 0, sizeof(*list));

	while (sceIoDread(dir, &dirent) > 0) {

		file_list_entry *entry = malloc(sizeof(*entry));

		strcpy(entry->name, dirent.d_name);
		entry->is_dir = SCE_S_ISDIR(dirent.d_stat.st_mode);
		if (!entry->is_dir) {
			entry->supported = file_supported(entry->name, supported_ext);
		}

		file_list_add_entry(list, entry);

		memset(&dirent, 0, sizeof(dirent));
	}

	sceIoDclose(dir);

	file_list_entry *up = malloc(sizeof(*up));
	strcpy(up->name, "..");
	up->is_dir = 1;
	up->next = NULL;
	file_list_add_entry(list, up);

	return 0;
}

static file_list_entry *file_list_get_nth_entry(const file_list *list, int n)
{
	file_list_entry *entry = list->head;
	while (n--) {
		entry = entry->next;
	}
	return entry;
}

static void dir_up(char *path)
{
	size_t len_in = strlen(path);
	char *pch = strrchr(path, '/');
	if (pch) {
		size_t s = len_in - (pch - path);
		memset(pch, '\0', s);
	}
	if (strcmp(path, "ux0:") < 0) {
		strcpy(path, "ux0:");
	}
}

int file_choose(const char *start_path, char *chosen_file, const char *title, const char *supported_ext[])
{
	SceCtrlData pad, old_pad;
	unsigned int keys_down;
	int i;
	int selected = 0;
	char cur_path[PATH_MAX];

	pad.buttons = old_pad.buttons = 0;

	strcpy(cur_path, start_path);

	file_list list;
	file_list_entry *entry;

	file_list_build(cur_path, &list, supported_ext);

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		keys_down = pad.buttons & ~old_pad.buttons;

		if (pad.buttons & GAME_EXIT_COMBO)
			return -1;

		if (keys_down & SCE_CTRL_UP) {
			selected--;
			if (selected < list.scroll) {
				list.scroll--;
			}
			if (selected < 0) {
				selected = list.length - 1;
				list.scroll = max(0, list.length - LIST_MAX_ONSCREEN);
			}
		} else if (keys_down & SCE_CTRL_DOWN) {
			selected++;
			if (selected == list.scroll + LIST_MAX_ONSCREEN) {
				list.scroll++;
			}
			if (selected == list.length) {
				selected = 0;
				list.scroll = 0;
			}
		}

		if (keys_down & (SCE_CTRL_CROSS | SCE_CTRL_START)) {
			file_list_entry *entry = file_list_get_nth_entry(&list, selected);

			if (entry->is_dir) {
				if (strcmp(entry->name, "..") == 0) {
					dir_up(cur_path);
				} else {
					char new_path[PATH_MAX];
					sprintf(new_path, "%s/%s", cur_path, entry->name);
					strcpy(cur_path, new_path);
				}
				file_list_empty(&list);
				file_list_build(cur_path, &list, supported_ext);
				selected = 0;
			} else if (entry->supported) {
				sprintf(chosen_file, "%s/%s", cur_path, entry->name);
				file_list_empty(&list);
				return 1;
			}
		} else if (keys_down & SCE_CTRL_CIRCLE) {
			dir_up(cur_path);
			file_list_empty(&list);
			file_list_build(cur_path, &list, supported_ext);
			selected = 0;
		}


		vita2d_start_drawing();
		vita2d_clear_screen();

		font_draw_stringf(10, 10, WHITE, title);

		entry = file_list_get_nth_entry(&list, list.scroll);
		for (i = list.scroll; i < min(list.length, list.scroll + LIST_MAX_ONSCREEN); i++) {

			font_draw_stringf(
				10,
				40 + (i - list.scroll)*20,
				(!entry->is_dir && entry->supported) ? GREEN : WHITE,
				"%s %s",
				(selected == i) ? ">" : "",
				entry->name);

			entry = entry->next;
		}

		old_pad = pad;
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}

	return 0;
}
