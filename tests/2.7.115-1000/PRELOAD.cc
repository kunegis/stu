#include <errno.h>
#include <sys/types.h>

extern "C"
pid_t fork()
{
	errno = ENOSYS;
	return -1;
}
