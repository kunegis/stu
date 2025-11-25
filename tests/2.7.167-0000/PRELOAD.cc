#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

int fd_main= -1;
void *addr_main= nullptr;

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
void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset)
{
	void *ret= ((void * (*)(void *, size_t, int, int, int, off_t))dlsym(
			RTLD_NEXT, "mmap"))(addr, length, prot, flags, fd, offset);
	if (fd == fd_main){
		addr_main= ret;
	}
	return ret;
}

extern "C"
int munmap(void *addr, size_t length)
{
	if (addr && addr == addr_main) {
		errno= EINVAL;
		return -1;
	}
	return ((int (*)(void *, size_t))dlsym(RTLD_NEXT, "munmap"))(addr, length);
}
