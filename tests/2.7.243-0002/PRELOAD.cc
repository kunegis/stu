#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include "strhash.hh"

static bool active= false;

extern "C"
void *realloc(void *ptr, size_t size)
{
	if (errno == strhash("File_Executor::execute::1")) {
		errno= ENOMEM;
		return NULL;
	}
	return ((void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc"))
		(ptr, size);
}
