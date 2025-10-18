#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int fd_schtroumpf= -1;

extern "C"
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *ret= ((FILE * (*)(const char *, const char *))dlsym(RTLD_NEXT, "fopen"))
		(pathname, mode);
	if (ret && !strcmp(pathname, "SCHTROUMPF")) {
		fd_schtroumpf= fileno(ret);
	}
	return ret;
}

extern "C"
int fstat(int fd, struct stat *buf)
{
	if (fd_schtroumpf >= 0 && fd == fd_schtroumpf) {
		errno= ENOMEM;
		return -1;
	}
	return ((int (*)(int, struct stat *))dlsym(RTLD_NEXT, "fstat"))(fd, buf);
}
