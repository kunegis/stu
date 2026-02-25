#ifndef COV_HASH_HH
#define COV_HASH_HH

#include <string.h>

/*
 * Most trivial "hash function" that will distinguish the different arguments to
 * cov_tag().
 *
 * This is inline because we also include it from PRELOAD.cc files.
 */
inline int cov_hash(const char *s)
{
	int ret= 0;
	size_t len= strlen(s);
	for (size_t i= 0; i < len; ++i)
		((unsigned char *)&ret)[i % sizeof(int)]
			+= ((unsigned)s[i]) * (i+1) + i;
	return ret;
}

#endif /* ! COV_HASH_HH */
