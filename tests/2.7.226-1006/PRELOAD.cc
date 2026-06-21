#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static FILE *file_schtroumpf= nullptr;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *ret= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (ret && !strcmp(pathname, "SCHTROUMPF")) {
		file_schtroumpf= ret;
	}
	return ret;
}

extern "C"
int fclose(FILE *stream)
{
	if (file_schtroumpf && stream == file_schtroumpf) {
		file_schtroumpf= nullptr;
		errno= ENOMEM;
		return EOF;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "fclose"))(stream);
}
