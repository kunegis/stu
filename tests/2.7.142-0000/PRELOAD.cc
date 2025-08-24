#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static FILE *b= NULL;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *ret= ((FILE * (*)(const char *, const char *))dlsym(
			RTLD_NEXT, "fopen"))(pathname, mode);
	if (!strcmp(pathname, "B"))
		b= ret;
	return ret;
}

extern "C"
int fclose(FILE *stream)
{
	int ret= ((int (*)(FILE *))dlsym(RTLD_NEXT, "fclose"))(stream);
	if (stream == b) {
		errno= EIO;
		ret= -1;
	}
	return ret;
}
