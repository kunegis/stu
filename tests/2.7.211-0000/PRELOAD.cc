#include <errno.h>

extern "C"
int sigwait(void *, int *)
{
	return EINVAL;
}
