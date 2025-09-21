#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

static int fd_Y= -1;

extern "C"
int open(const char *pathname, int flags, mode_t mode)
{
	int r= ((int (*)(const char *, int, mode_t))
		dlsym(RTLD_NEXT, "open"))(pathname, flags, mode);
	if (!strcmp(pathname, "Y")) {
		fd_Y= r;
	}
	return r;
}

extern "C"
int close(int fd)
{
	if (fd == fd_Y) {
		errno= EIO;
		return -1;
	}
	return ((int (*)(int))dlsym(RTLD_NEXT, "close"))(fd);
}
