#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>

#include <cov_hash.hh>

extern "C"
void *malloc(size_t size)
{
	if (errno == cov_hash("Job_List::create_child_env")) {
		errno= ENOMEM;
		return nullptr;
	}
	return ((void * (*)(size_t))dlsym(RTLD_NEXT, "malloc"))(size);
}
