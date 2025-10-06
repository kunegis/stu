#include <errno.h>
#include <sys/types.h>

extern "C"
pid_t fork()
{
	errno= ENOMEM;
	return -1;
}
