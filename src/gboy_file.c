#include "gboy.h"

void
create_dir(char *dir_name, mode_t mode_mask)
{
	if ( (mkdir(dir_name, mode_mask)) == -1)
		perror("Create Directory: ");
}

void
change_cur_dir(char *dir_path)
{
	if ( (chdir(dir_path)) == -1)
		perror("Change Directory: ");
}

char *
get_home_path()
{
	static char *ptr_hpath;

	/* XXX Can this fail? */
	if ( (ptr_hpath = getenv("HOME")) == NULL)
		perror("Home directory: ");

	return ptr_hpath;
}

int
search_file_dir(char *file, char *dir_path)
{
	DIR *home_dir = opendir(dir_path);
	struct dirent *dir_ent;
	while ( (dir_ent=readdir(home_dir)) != NULL) {
		if (!(strncmp(dir_ent->d_name, ".realboy", 9)))
			break;
	}
	closedir(home_dir);

	if (dir_ent != NULL)
		return 1;
	else
		return 0;
}
