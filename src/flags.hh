#ifndef FLAGS_HH
#define FLAGS_HH

/*
 * Flags apply to dependencies.  Flags are binary and option-like, and appear at multiple
 * levels in Stu, from Stu source code where they are represented by a syntax ressembling
 * that of command line flags, to attributes of edges in the dependency graph. Internally,
 * flags are defined as bit fields.
 *
 * Each edge in the dependency graph is annotated with one object of this type.  This
 * contains bits related to what should be done with the dependency, whether time is
 * considered, etc.  The flags are defined in such a way that the simplest dependency is
 * represented by zero, and each flag enables a specific feature.
 *
 * The placed bits are effectively set for tasks not to do.  Therefore, inverting them
 * gives the bits for the tasks to do.  Other flags have the semantics of "more to do".
 */

#include "show.hh"

typedef unsigned Flags;

enum
{
	/* The index of the flags (I_*), used for array indexing.  Variables
	 * iterating over these values are usually called I. */
	I_PERSISTENT= 0,      /* -p  \                  \                    */
	I_OPTIONAL,           /* -o   | placed flags     |                   */
	I_TRIVIAL,            /* -t  /                   |                   */
	I_TARGET_DYNAMIC,     /* [ ] \  target flags     |                   */
	I_TARGET_TRANSIENT,   /* @   /                   | target word flags */
	I_VARIABLE,           /* $                       |                   */
	I_NEWLINE_SEPARATED,  /* -n  \                   |                   */
	I_NUL_SEPARATED,      /* -0   | attribute flags  |                   */
	I_CODE,               /* -C  /                  /                    */
	I_INPUT,              /* <                                           */
	I_RESULT_NOTIFY,      /* -*                                          */
	I_RESULT_COPY,        /* -%                                          */
	I_PHASE_B,            /* -B                                          */

	/* Counts */
	C_ALL,
	C_PLACED    = 3,  /* Flags for which we store a place in Dep */
	C_WORD      = 9,  /* Flags used for caching; stored in Hash_Dep */

	/*
	 * Flag bits to be ORed together
	 */

	F_PERSISTENT            = 1 << I_PERSISTENT,
	/* (-p) When the dependency is newer than the target, don't rebuild */

	F_OPTIONAL              = 1 << I_OPTIONAL,
	/* (-o) Don't create the dependency if it doesn't exist */

	F_TRIVIAL               = 1 << I_TRIVIAL,
	/* (-t) Trivial dependency */

	F_TARGET_DYNAMIC        = 1 << I_TARGET_DYNAMIC,
	/* A dynamic target */

	F_TARGET_TRANSIENT      = 1 << I_TARGET_TRANSIENT,
	/* A transient target */

	F_VARIABLE              = 1 << I_VARIABLE,
	/* ($[...]) Content of file is used as variable */

	F_NEWLINE_SEPARATED     = 1 << I_NEWLINE_SEPARATED,
	/* For dynamic dependencies, the file contains newline-separated
	 * filenames, without any markup. */

	F_NUL_SEPARATED         = 1 << I_NUL_SEPARATED,
	/* For dynamic dependencies, the file contains NUL-separated filenames,
	 * without any markup. */

	F_CODE                  = 1 << I_CODE,
	/* For dynamic dependencies, the file contains Stu code */

	F_INPUT                 = 1 << I_INPUT,
	/* A dependency is annotated with the input redirection flag '<' */

	F_RESULT_NOTIFY         = 1 << I_RESULT_NOTIFY,
	/* The link A ---> B between two executors annotated with this flag
	 * means that A is notified of B's results. */

	F_RESULT_COPY           = 1 << I_RESULT_COPY,
	/* The link A ---> B between two executors annotated with this
	 * flags means that the results of B will be copied into A's result. */

	F_PHASE_B               = 1 << I_PHASE_B,
	/* A parent is in phase B */

	/*
	 * Aggregates
	 */
	F_PLACED        = (1 << C_PLACED) - 1,
	F_WORD          = (1 << C_WORD) - 1,
	F_TARGET        = F_TARGET_DYNAMIC | F_TARGET_TRANSIENT,
	F_ATTRIBUTE     = F_NEWLINE_SEPARATED | F_NUL_SEPARATED | F_CODE,
	F_RESULT        = F_RESULT_NOTIFY | F_RESULT_COPY,
};

extern const char flags_chars[];
extern const char *flags_phrases[C_PLACED];

unsigned flag_get_index(char c);
bool render_flags(Flags flags, Parts &, Rendering= 0);
string show_flags(Flags, Style= S_DEFAULT);

#endif /* ! FLAGS_HH */
