#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static FILE *stream_schtroumpf= nullptr;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *stream= ((FILE * (*)(const char *, const char *))
		dlsym(RTLD_NEXT, "fopen"))(pathname, mode);
	fprintf(stderr, ">>> fopen pathname='%s' mode='%s' stream=%p\n", pathname, mode, stream); // rm
	if (stream && !strcmp(pathname, "SCHTROUMPF")) {
		stream_schtroumpf= stream;
	}
	return stream;
}

extern "C"
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	fprintf(stderr, ">>> fwrite ptr=%p size=%zu nmemb=%zu stream=%p\n", ptr, size, nmemb, stream); // rm
	if (stream_schtroumpf && stream == stream_schtroumpf) {
		errno= ENOSPC;
		return 0;
	}
	return ((size_t (*)(const void *, size_t, size_t, FILE *))
		dlsym(RTLD_NEXT, "fwrite"))(ptr, size, nmemb, stream);
}

extern "C"
int ferror(FILE *stream) 
{
	fprintf(stderr, ">>> ferror stream=%p\n", stream); // rm
	if (stream_schtroumpf && stream == stream_schtroumpf) {
		stream_schtroumpf= nullptr;
		return 1;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "ferror"))(stream);
}

extern "C"
int fclose(FILE *stream)
{
	fprintf(stderr, ">>> fclose stream=%p\n", stream); // rm
	if (stream_schtroumpf && stream == stream_schtroumpf) {
		stream_schtroumpf= nullptr;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "fclose"))(stream);
}
