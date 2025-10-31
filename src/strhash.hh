#ifndef STRHASH_H
#define STRHASH_H

#include <string.h>

int strhash(const char *s)
{
	int ret= 0;
	size_t len= strlen(s);
	for (size_t i= 0; i < len; ++i)
		((unsigned char *)&ret)[i % sizeof(int)] += s[i];
	return ret;
}

#endif /* ! STRHASH_H */
