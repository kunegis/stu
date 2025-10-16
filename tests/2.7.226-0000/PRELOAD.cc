#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int open(const char *pathname, int flags)
{
	if (!strcmp(pathname, "x")) {
		errno= EACCES;
		return -1;
	}
	return ((int (*)(const char *, int))dlsym(RTLD_NEXT, "open"))(pathname, flags);
}
