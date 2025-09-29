#include <dlfcn.h>
#include <errno.h>
#include <signal.h>

extern "C"
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	if (how == SIG_UNBLOCK && sigismember(set, SIGTERM)) {
		errno= EINVAL;
		return -1;
	}

	return ((int (*)(int, const sigset_t *, sigset_t *))dlsym(RTLD_NEXT, "sigprocmask"))
		(how, set, oldset);
}
