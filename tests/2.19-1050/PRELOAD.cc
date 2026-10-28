#include <errno.h>
#include <stddef.h>
#include <string.h>

constexpr size_t len= 1500;

char name[len + 1] = "";

extern "C"
char *getcwd(char *buf, size_t size)
{
	if (!buf) {
		errno= EINVAL;
		return nullptr;
	}

	if (!*name) {
		name[0]= '/';
		memset(name+1, 'x', len-1);
		name[len]= '\0';
	}

	if (size < len + 1) {
		errno= ERANGE;
		return nullptr;
	}

	return strcpy(buf, name);
}
