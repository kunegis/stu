#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

static int fd_saved= -1;

extern "C"
int openat(int dirfd, const char *pathname, int flags, mode_t mode)
{
	int ret= ((int (*)(int, const char *, int, mode_t))dlsym(
			RTLD_NEXT, "openat"))(dirfd, pathname, flags, mode);
	if (!strcmp(pathname, "main.stu"))
		fd_saved= ret;
	return ret;
}

extern "C"
int fstat(int fd, void *statbuf)
{
	int ret= ((int (*)(int, void *))dlsym(RTLD_NEXT, "fstat"))(fd, statbuf);
	if (fd == fd_saved) {
		errno= EIO;
		ret= -1;
		fd_saved= -1;
	}
	return ret;
}
