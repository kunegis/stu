#ifndef BITS_HH
#define BITS_HH

/*
 * These are bits set for individual executor objects.  The semantics of each
 * is chosen such that in a new executor object, the value is zero.  The
 * semantics of the different bits are distinct and could just as well be
 * realized as individual "bool" variables.
 */

typedef unsigned Bits;

enum {
	B_NEED_BUILD  = 1 << 0,
	/* Whether this target needs to be built.  When a target is finished,
	 * this value is propagated to the parent executors. */

	B_CHECKED     = 1 << 1,
	/* Whether a certain check has been performed.  Only used by
	 * File_Executor. */

	B_EXISTING    = 1 << 2,
	/* All file targets are known to exist (only in File_Executor; may be
	 * set when there are no file targets). */

	B_MISSING     = 1 << 3,
	/* At least one file target is known not to exist (only possible if
	 * there is at least one file target in File_Executor).  */
};

#endif /* ! BITS_HH */
