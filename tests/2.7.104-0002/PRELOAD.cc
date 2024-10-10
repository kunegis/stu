#include <errno.h>

struct timeval;
struct timezone;

extern "C"
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	errno= EINVAL;
	return -1;
}
