#include <dlfcn.h>
#include <errno.h>
#include <signal.h>

extern "C"
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	if (signum == SIGUSR1) {
		errno= EFAULT;
		return -1;
	}
	return ((int (*)(int, const struct sigaction *, struct sigaction *))
		dlsym(RTLD_NEXT, "sigaction"))(signum, act, oldact);
}
