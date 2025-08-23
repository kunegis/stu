#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>

extern "C"
int fclose(FILE *stream)
{
	if (stream == stdout){
		errno= EBADF;
		return -1;
	} else {
		return ((int (*)(FILE *))dlsym(RTLD_NEXT, "fclose"))(stream);
	}
}
