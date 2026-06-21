#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static FILE *file_A= nullptr;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *ret= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (!strcmp(pathname, "A")) {
		file_A= ret;
	}
	return ret;
}

extern "C"
int fclose(FILE *stream)
{
	int ret= ((int (*)(FILE *))dlsym(RTLD_NEXT, "fclose"))(stream);
	if (stream == file_A) {
		file_A= nullptr;
		errno= ENOSPC;
		return EOF;
	}
	return ret;
}
