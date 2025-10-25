#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>

static bool done= false;

extern "C"
pid_t wait(int *wstatus)
{
	if (!done) {
		done= true;
		errno= EINTR;
		return -1;
	}
	return ((pid_t (*)(int *))dlsym(RTLD_NEXT, "wait"))(wstatus);
}
