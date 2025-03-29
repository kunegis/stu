#include <errno.h>
#include <time.h>

struct timespec;

extern "C"
int clock_gettime(clockid_t, struct timespec *)
{
	errno= EINVAL;
	return -1;
}
