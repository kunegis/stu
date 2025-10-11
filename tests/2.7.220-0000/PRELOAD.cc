#include <errno.h>

extern "C"
int sigismember(void *set, int signum)
{
	errno= EINVAL;
	return -1;
}
