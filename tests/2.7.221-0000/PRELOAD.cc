#include <errno.h>

extern "C"
int getrusage(int, void *)
{
	errno= EFAULT;
	return -1;
}
