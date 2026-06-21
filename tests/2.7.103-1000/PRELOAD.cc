#include <stdio.h>
#include <stddef.h>
#include <errno.h>

extern "C"
int setvbuf(FILE *, char *, int, size_t)
{
	errno = ENOMEM;
	return -1;
}
