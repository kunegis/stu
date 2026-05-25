#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>

extern "C"
int sigemptyset(void *p)
{
	static int n= -1;
	if (n < 0) {
		n= atoi(getenv("STU_N"));
	}

	static int k= 0;
	if (k++ == n) {
		errno= EINVAL;
		return -1;
	}

	return ((int (*)(void *))dlsym(RTLD_NEXT, "sigemptyset"))(p);
}
