#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

constexpr const char *pathname_schtroumpf= "SCHTROUMPF";

static FILE *stream_schtroumpf= nullptr;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *stream= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (!stream_schtroumpf && !strcmp(pathname, pathname_schtroumpf)) {
		stream_schtroumpf= stream;
	}
	return stream;
}

extern "C"
int fputc(int c, FILE *stream)
{
	if (stream_schtroumpf && stream == stream_schtroumpf) {
		stream_schtroumpf= nullptr;
		errno= ENOSPC;
		return EOF;
	}
	return ((int (*)(int, FILE *))dlsym(RTLD_NEXT, "fputc"))(c, stream);
}

extern "C"
int remove(const char *pathname)
{
	if (!strcmp(pathname, "SCHTROUMPF")) {
		errno= EBUSY;
		return -1;
	}
	return ((int (*)(const char *))dlsym(RTLD_NEXT, "remove"))(pathname);
}
