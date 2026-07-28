#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "cov_hash.hh"

extern "C"
int sprintf(char *str, const char *format, ...)
{
	va_list args;

	if (errno == cov_hash("Job_List::create_child_env::2")) {
		errno= ENOMEM;
		return -1;
	}

	va_start(args, format);
	int r = vsprintf(str, format, args);
	va_end(args);
	return r;
}
