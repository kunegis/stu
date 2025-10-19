#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int unlink(const char *pathname)
{
	if (!strcmp(pathname, "SCHTROUMPF")) {
		errno= EACCES;
		return -1;
	}
	return ((int (*)(const char *))dlsym(RTLD_NEXT, "unlink"))(pathname);
}
