#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

static int fd_main= -1;

extern "C"
int open(const char *pathname, int flags, mode_t mode)
{
	int ret= ((int (*)(const char *, int, mode_t))dlsym(
			RTLD_NEXT, "open"))(pathname, flags, mode);
	if (!strcmp(pathname, "main.stu"))
		fd_main= ret;
	return ret;
}

extern "C"
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (fd == fd_main) {
		errno= ENODEV;
		fd_main= -1;
		return MAP_FAILED;
	}
	void *ret= ((void * (*)(void *, size_t, int, int, int, off_t))dlsym(
			RTLD_NEXT, "mmap"))(addr, length, prot, flags, fd, offset);
	return ret;
}
