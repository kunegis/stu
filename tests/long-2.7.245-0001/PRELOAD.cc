#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

extern "C"
int open(const char *pathname, int flags, mode_t mode)
{
	if (!strcmp(pathname, "/dev/tty")) {
		errno= ENOENT;
		return -1;
	}
	return ((int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open"))
		(pathname, flags, mode);
}
