#include <dlfcn.h>
#include <errno.h>

extern "C"
int fstat(int fd, void *p)
{
	errno= ELOOP;
	if (fd <=2) {
		return ((int (*)(int, void *))dlsym(RTLD_NEXT, "fstat"))(fd, p);
	} else {
		return -1;
	}
}
