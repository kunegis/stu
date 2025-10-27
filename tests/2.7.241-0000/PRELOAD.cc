#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int stat(const char *pathname, struct stat *statbuf)
{
	if (!strcmp(pathname, "A")) {
		errno= EACCES;
		return -1;
	}
	return ((int (*)(const char *, struct stat *))dlsym(RTLD_NEXT, "stat"))
		(pathname, statbuf);
}
