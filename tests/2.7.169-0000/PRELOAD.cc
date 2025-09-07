#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int execv(const char *pathname, char *const argv[])
{
	if (!strcmp(pathname, "/bin/cp")) {
		errno= EPERM;
		return -1;
	}
	return ((int (*)(const char *, char *const[]))dlsym(RTLD_NEXT, "execv"))
		(pathname, argv);
}
