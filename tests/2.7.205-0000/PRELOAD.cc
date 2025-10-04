#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
char *strdup(const char *s)
{
	if (!strcmp(s, "A")) {
		errno= ENOMEM;
		return nullptr;
	}

	return ((char * (*)(const char *))dlsym(RTLD_NEXT, "strdup"))(s);
}
