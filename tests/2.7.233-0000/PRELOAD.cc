#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int lstat(const char *pathname, struct stat *statbuf)
{
	if (!strcmp(pathname, "X")) {
		errno= ENOENT;
		return -1;
	}
	return ((int (*)(const char *, struct stat *))dlsym(RTLD_NEXT, "lstat"))
		(pathname, statbuf);
}
