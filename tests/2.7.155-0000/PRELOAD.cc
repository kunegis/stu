#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

extern "C"
int open(const char *pathname, int flags)
{
	if (!strcmp(pathname, "/dev/tty")) {
		abort();
	}
	return ((int (*)(const char *, int))dlsym(
			RTLD_NEXT, "open"))(pathname, flags);
}
