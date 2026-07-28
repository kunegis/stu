#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int putenv(const char *string)
{
	if (string && !strcmp(string, "abcdef=xyz")) {
		errno= ENOMEM;
		return -1;
	}

	return ((int (*)(const char *))dlsym(RTLD_NEXT, "putenv"))(string);
}
