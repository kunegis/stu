#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

static int fd_ddd= -1;

extern "C"
int open(const char *pathname, int flags, mode_t mode)
{
	int ret= ((int (*)(const char *, int, mode_t))dlsym(
			RTLD_NEXT, "open"))(pathname, flags, mode);
	if (!strcmp(pathname, "ddd"))
		fd_ddd= ret;
	return ret;
}

extern "C"
int close(int fd)
{
	int ret= ((int (*)(int))dlsym(RTLD_NEXT, "close"))(fd);
	if (fd == fd_ddd) {
		errno= EIO;
		ret= -1;
		fd_ddd= -1;
	}
	return ret;
}
