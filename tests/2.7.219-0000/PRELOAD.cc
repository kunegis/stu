#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>

int get_i()
{
	static int i= -1;
	if (i < 0) {
		const char *env= getenv("STU_I");
		if (!env) exit(99);
		i= atoi(env);
		if (i == 0) exit(99);
	}
	return i;
}

bool err_case()
{
	static int count= 0;
	return ++count == get_i();
}

extern "C"
int sigaddset(sigset_t *set, int signum)
{
	if (err_case()) {
		errno= EINVAL;
		return -1;
	}

	return ((int (*)(sigset_t *, int))dlsym(RTLD_NEXT, "sigaddset"))(set, signum);
}
