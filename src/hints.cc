#include "hints.hh"

#if !defined(NDEBUG) || defined(STU_COV)

#include "cov_hash.hh"

void cov_tag(const char *tag)
{
	errno= cov_hash(tag);
}

#endif /* !NDEBUG || STU_COV */
