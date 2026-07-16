#include <stdlib.h>
#include <string.h>

extern "C"
struct passwd *getpwnam(const char *name)
{
	if (strcmp(name, "nonexistinguser")) {
		abort();
	}

	return nullptr;
}
