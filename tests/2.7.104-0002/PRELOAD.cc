#include <dlfcn.h>
#include <errno.h>
#include <time.h>

struct timespec;

extern "C"
int clock_gettime(clockid_t clockid, struct timespec *tp)
{
	if (clockid == CLOCK_REALTIME) {
		/* options.cc */
		errno= EINVAL;
		return -1;
	} else {
		/* timestamp.cc */
		return ((int (*)(clockid_t, struct timespec *))dlsym(
				RTLD_NEXT, "clock_gettime"))(clockid, tp);
	}
}
