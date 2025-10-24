#include <dlfcn.h>
#include <signal.h>

extern "C"
int raise(int sig)
{
	if (sig == SIGQUIT)
		return -1;
	return ((int (*)(int))dlsym(RTLD_NEXT, "raise"))(sig);
}
