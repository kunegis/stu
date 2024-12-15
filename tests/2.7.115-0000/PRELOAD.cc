#include <errno.h>

extern "C"
int fork()
{
	errno = ENOSYS;
	return -1;
}
