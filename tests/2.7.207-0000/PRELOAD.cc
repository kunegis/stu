#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *file= NULL;
static bool error= false;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *ret= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (!strcmp(pathname, "list.ABC")) {
		file= ret;
	}
	return ret;
}

extern "C"
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (file && stream == file) {
		error= true;
		errno= ENOMEM;
		return 0;
	}

	return ((size_t (*)(const void *, size_t, size_t, FILE *))
		dlsym(RTLD_NEXT, "fwrite"))(ptr, size, nmemb, stream);
}

extern "C"
int ferror(FILE *stream)
{
	if (file && stream == file && error) {
		return 1;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "ferror"))(stream);
}
