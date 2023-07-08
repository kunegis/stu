#ifndef HINTS_HH
#define HINTS_HH

/*
 * Used in branches that we expect not to never happen, but could happen when
 * Stu has a bug, and we want to handle that case even in non-debug mode.
 */

#ifdef NDEBUG

#define should_not_happen()

#ifdef __GNUC__
#	define unreachable() __builtin_unreachable();
#else
#	define unreachable()
#endif

#else /* ! NDEBUG */

#include <stdio.h>

#define should_not_happen()						\
	do {								\
		fprintf(stderr, "%s:%d: should not happen\n",	\
			__FILE__, __LINE__);				\
	} while (0)

#define unreachable() assert(0)

#endif /* ! NDEBUG */

#endif /* ! HINTS_HH */
