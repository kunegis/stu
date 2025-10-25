#include <errno.h>
#include <sys/types.h>

extern "C"
pid_t wait(int *wstatus)
{
	errno= EINVAL;
	return -1;
}
