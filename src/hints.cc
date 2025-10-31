#include "hints.hh"

#if !defined(NDEBUG) || defined(STU_COV)

void cov_tag(const char *tag)
{
	errno= strhash(tag);
}

#endif
