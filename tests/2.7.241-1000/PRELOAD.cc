#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
	if (!strcmp(pathname, "A")) {
		errno= EACCES;
		return -1;
	}
	return ((int (*)(int, const char *, struct stat *, int))dlsym(RTLD_NEXT, "fstatat"))
		(dirfd, pathname, statbuf, flags);
}
