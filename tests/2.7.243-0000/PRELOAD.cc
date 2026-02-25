#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include "cov_hash.hh"

static bool active= false;

extern "C"
void *realloc(void *ptr, size_t size)
{
	if (errno == cov_hash("File_Executor::execute")) {
		errno= ENOMEM;
		return nullptr;
	}
	return ((void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc"))
		(ptr, size);
}
