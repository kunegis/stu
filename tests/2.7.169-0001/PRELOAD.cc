#include <dlfcn.h>
#include <errno.h>
#include <string.h>

extern "C"
int execve(const char *pathname, char *const argv[], char *const envp[])
{
	if (!strcmp(pathname, "/bin/sh")) {
		errno= EPERM;
		return -1;
	}
	return ((int (*)(const char *, char *const[], char *const[]))dlsym
		(RTLD_NEXT, "execve"))(pathname, argv, envp);
}
