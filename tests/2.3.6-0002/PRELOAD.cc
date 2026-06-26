#include <errno.h>
#include <unistd.h>

extern "C"
int tcsetpgrp(int fd, pid_t pgrp)
{
	errno= EPERM;
	return -1;
}
