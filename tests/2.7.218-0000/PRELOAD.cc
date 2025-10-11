#include <errno.h>

extern "C"
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	errno= EFAULT;
	return -1;
}
