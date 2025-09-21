#include <errno.h>

extern "C"
int dup2(int oldfd, int newfd)
{
	errno= EINVAL;
	return -1;
}
