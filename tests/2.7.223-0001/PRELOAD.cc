#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

extern "C"
int kill(pid_t pid, int sig)
{
	if (sig == SIGTERM) {
		errno= ESRCH;
		return -1;
	}
	return ((int (*)(pid_t, int))dlsym(RTLD_NEXT, "kill"))(pid, sig);
}
