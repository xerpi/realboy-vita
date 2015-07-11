#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include "utils.h"

/* libc functions */

char current_dir[1024];

size_t strnlen(register const char *s, size_t maxlen)
{
	register const char *e;
	size_t n;

	for (e = s, n = 0; *e && n < maxlen; e++, n++)
		;
	return n;
}

char *strndup(const char *s, size_t n)
{
	char *result;
	size_t len = strlen (s);

	if (n < len)
		len = n;

	result = (char *) malloc (len + 1);
	if (!result)
		return 0;

	result[len] = '\0';
	return (char *) memcpy (result, s, len);
}

char *basename(const char *name)
{
	const char *base = name;

	while (*name) {
		if (*name++ == '/') {
			base = name;
		}
	}
	return (char *) base;
}

int chdir(const char *path)
{
	if (strcmp(path, "..") == 0) {
		const char *new_path = dirname(current_dir);
		strcpy(current_dir, new_path);
	} else {
		if (strstr(path, "cache0:/")) {
			strcpy(current_dir, path);
		} else {
			char new_path[1024];
			sprintf(new_path, "%s/%s", current_dir, path);
			strcpy(current_dir, new_path);
		}
	}
	return 0;
}

char *getenv(const char *name)
{
	if (strcmp(name, "HOME") == 0) {
		return "cache0:/VitaDefilerClient/Documents";
	}
	return NULL;
}

void *memrchr(const void *s, int c, size_t n)
{
	const unsigned char *cp;

	if (n != 0) {
		cp = (unsigned char *)s + n;
		do {
			if (*(--cp) == (unsigned char)c)
				return((void *)cp);
		} while (--n != 0);
	}
	return(NULL);
}

// Stolen from http://www.ic.unicamp.br/~islene/2s2008-mo806/libc/misc/dirname.c

char *dirname(char *path)
{
  static const char dot[] = ".";
  char *last_slash;

  /* Find last '/'.  */
  last_slash = path != NULL ? strrchr (path, '/') : NULL;

  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0')
    {
      /* Determine whether all remaining characters are slashes.  */
      char *runp;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != '/')
	  break;

      /* The '/' is the last character, we have to look further.  */
      if (runp != path)
	last_slash = memrchr (path, '/', runp - path);
    }

  if (last_slash != NULL)
    {
      /* Determine whether all remaining characters are slashes.  */
      char *runp;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != '/')
	  break;

      /* Terminate the path.  */
      if (runp == path)
	{
	  /* The last slash is the first character in the string.  We have to
	     return "/".  As a special case we have to return "//" if there
	     are exactly two slashes at the beginning of the string.  See
	     XBD 4.10 Path Name Resolution for more information.  */
	  if (last_slash == path + 1)
	    ++last_slash;
	  else
	    last_slash = path + 1;
	}
      else
	last_slash = runp;

      last_slash[0] = '\0';
    }
  else
    /* This assignment is ill-designed but the XPG specs require to
       return a string containing "." in any case no directory part is
       found and so a static and constant string is required.  */
    path = (char *) dot;

  return path;
}
