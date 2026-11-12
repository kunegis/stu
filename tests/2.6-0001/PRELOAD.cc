#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int count= 0;

extern "C"
int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
	if (! strcmp(pathname, "./B")) {
		fprintf(stderr, "*** PRELOAD: fstatat must not be called with './B'\n");
		exit(77);
	}
	if (! strcmp(pathname, "B")) {
		++count;
	}

	return ((int (*)(int, const char *, struct stat *, int))dlsym(
			RTLD_NEXT, "fstatat"))(dirfd, pathname, statbuf, flags);
}

extern "C"
int execv(const char *pathname, char *const argv[])
{
        unsetenv("LD_PRELOAD");
        int ret= ((int (*)(const char *, char *const[]))
                dlsym(RTLD_NEXT, "execv"))(pathname, argv);
        return ret;
}

extern "C"
__attribute__((destructor))
void preload_destructor()
{
	if (count != 1) {
		fprintf(stderr, "*** PRELOAD: count must be 1, found count= %d\n", count);
		exit(78);
	}
}
