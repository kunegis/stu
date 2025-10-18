#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *stream_abc= nullptr;
static bool error= false;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *stream= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (!strcmp(pathname, "list.ABC")) {
		stream_abc= stream;
	}
	return stream;
}

extern "C"
int fputc(int c, FILE *stream)
{
	if (stream_abc && stream == stream_abc) {
		error= true;
		errno= ENOMEM;
		return EOF;
	}

	return ((int (*)(int, FILE *))dlsym(RTLD_NEXT, "putc"))(c, stream);
}

extern "C"
int ferror(FILE *stream)
{
	if (stream_abc && stream == stream_abc && error) {
		return 1;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "ferror"))(stream);
}
