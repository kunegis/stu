#ifndef HINTS_HH
#define HINTS_HH

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
#else
#define __gcov_dump()
#endif

#endif /* ! HINTS_HH */
