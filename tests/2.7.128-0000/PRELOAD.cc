#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

extern "C"
void *mmap(void *, size_t, int, int, int, off_t)
{
	return MAP_FAILED;
}
