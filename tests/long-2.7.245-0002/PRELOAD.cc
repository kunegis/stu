#include <errno.h>
#include <sys/types.h>

extern "C"
pid_t tcsetpgrp(int fd)
{
	errno= EPERM;
	return -1;
}
