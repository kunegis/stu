#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

static bool next_feof_zero= false;

extern "C"
ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
	if (stream == stdin) {
		next_feof_zero= true;
		errno= ENOMEM;
		return -1;
	}
	return ((ssize_t (*)(char **, size_t *, FILE *))dlsym(RTLD_NEXT, "getline"))
		(lineptr, n, stream);
}

extern "C"
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
	if (stream == stdin) {
		next_feof_zero= true;
		errno= ENOMEM;
		return -1;
	}
	return ((ssize_t (*)(char **, size_t *, int, FILE *))dlsym(RTLD_NEXT, "getdelim"))(lineptr, n, delim, stream);
}

extern "C"
int feof(FILE *stream)
{
	if (next_feof_zero && stream == stdin) {
		next_feof_zero= false;
		return 0;
	}
	return ((int (*)(FILE *))dlsym(RTLD_NEXT, "feof"))(stream);
}
