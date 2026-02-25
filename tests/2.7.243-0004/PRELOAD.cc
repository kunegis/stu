#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>

#include "cov_hash.hh"

extern "C"
void *malloc(size_t size)
{
	if (errno == cov_hash("Job::create_child_env::1")) {
		errno= ENOMEM;
		return nullptr;
	}
	static void * (*f)(size_t)= nullptr;
	if (!f) {
		f= (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");
	}
	return f(size);
}
