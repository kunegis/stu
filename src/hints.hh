#ifndef HINTS_HH
#define HINTS_HH

/*
 * Macros to mark certain things in the code.
 */

#define happens_only_on_certain_platforms()
/* Used to mark branches that are only possible on certain platforms or architectures,
 * usually with certain combinations of type sizes. */

#ifdef NDEBUG

#define should_not_happen()
/* Used in branches that we expect not to never happen, but could happen when Stu has a
 * bug, and we want to handle that case even in non-debug mode. */

#ifdef __GNUC__
#	define unreachable() __builtin_unreachable()
#else
#	define unreachable()
#endif

#else /* ! NDEBUG */

#include <stdio.h>

#define should_not_happen()						\
	do {								\
		fprintf(stderr, "%s:%d: should not happen\n",		\
			__FILE__, __LINE__);				\
	} while (0)

#define unreachable() assert(0)

#endif /* ! NDEBUG */

#ifdef STU_COV

extern "C" {
#include <gcov.h>
}

/* Like __gcov_pre_dump(), but may be followed by _gcov_dump().  __gcov_dump() cannot be
 * called multiple times. */
#define __gcov_pre_dump() __gcov_dump(); __gcov_reset()

#else /* ! STU_COV */

#define __gcov_dump()
#define __gcov_pre_dump()

#endif /* ! STU_COV */

/* Used to mark specific location in the source code for testing.  Disabled in the NDEBUG
 * variant.  Sets errno to a hash of the given string.  Tests that use PRELOAD.cc can use
 * the same function to check that errno is set to the specific value. */
#if !defined(NDEBUG) || defined(STU_COV)
#include "strhash.hh"
void cov_tag(const char *tag);
#else
#define cov_tag(tag)
#endif

#endif /* ! HINTS_HH */
