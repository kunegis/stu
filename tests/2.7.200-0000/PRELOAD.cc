#include <dlfcn.h>
#include <errno.h>

extern "C"
int sigemptyset(void *p)
{
	/* Number of times sigemptyset() is called before it is called in the signal
	 * handler */
	constexpr int n= 5;

	static int k= 0;
	if (k++ == n) {
		errno= EINVAL;
		return -1;
	}

	return ((int (*)(void *))dlsym(RTLD_NEXT, "sigemptyset"))(p);
}
