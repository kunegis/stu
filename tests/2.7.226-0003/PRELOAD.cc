#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	if (!strcmp(pathname, "SCHTROUMPF")) {
		errno= EACCES;
		return nullptr;
	}

	return ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
}
