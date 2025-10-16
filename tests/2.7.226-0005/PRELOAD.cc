#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static FILE *file_schtroumpf= NULL;

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
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (file_schtroumpf && stream == file_schtroumpf) {
		file_schtroumpf= NULL;
		errno= ENOMEM;
		return 0;
	}
	return ((size_t (*)(void *, size_t, size_t, FILE *))dlsym(RTLD_NEXT, "fread"))
		(ptr, size, nmemb, stream);
}
