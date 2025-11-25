#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>

#include "strhash.hh"

extern "C"
void *malloc(size_t size)
{
	if (errno == strhash("File_Executors::executors_add")) {
		errno= ENOMEM;
		return nullptr;
	}
	static void * (*f)(size_t)= nullptr;
	if (!f) {
		f= (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");
	}
	return f(size);
}
