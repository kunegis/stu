#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>

static bool had_getrusage= false;

extern "C"
int getrusage(int who, struct rusage *usage)
{
	had_getrusage= true;
	return ((int (*)(int, struct rusage *))dlsym(RTLD_NEXT, "getrusage"))
		(who, usage);
}

extern "C"
int ferror(FILE *)
{
	return 1;
}
