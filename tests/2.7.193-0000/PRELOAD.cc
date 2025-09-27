#include <dlfcn.h>
#include <errno.h>
#include <signal.h>

typedef void (*sighandler_t)(int);

extern "C"
sighandler_t signal(int signum, sighandler_t handler)
{
	if (signum == SIGTTIN && handler == SIG_IGN) {
		errno= EINVAL;
		return SIG_ERR;
	}
	return ((sighandler_t (*)(int, sighandler_t))dlsym(RTLD_NEXT, "signal"))
		(signum, handler);
}
