#ifndef FLAGS_HH
#define FLAGS_HH

/*
 * Flags apply to dependencies.  Flags are binary option-like, and apply at
 * multiple levels in Stu, from Stu source code where they are represented by a
 * syntax ressembling that of command line flags, to attributes of edges in the
 * dependency graph. Internally, flags are defined as bit fields.
 *
 * Each edge in the dependency graph is annotated with one object of this type.
 * This contains bits related to what should be done with the dependency,
 * whether time is considered, etc.  The flags are defined in such a way that
 * the simplest dependency is represented by zero, and each flag enables a
 * specific feature.
 *
 * The transitive bits are effectively set for tasks not to do.  Therefore,
 * inverting them gives the bits for the tasks to do.  In particular, the flag
 * fields that store the information which part of a task has been done has
 * inverse semantics: They have a bit set when that part has been done, i.e.,
 * when the flag initially was not set.
 */

#include "show.hh"

typedef unsigned Flags;
/* Declared as integer so arithmetic can be performed on it */

enum
{
	/* The index of the flags (I_*), used for array indexing.  Variables
	 * iterating over these values are usually called I.  */
	I_PERSISTENT= 0,      /* -p  \                  \                     */
	I_OPTIONAL,           /* -o   | placed flags     |                    */
	I_TRIVIAL,            /* -t  /                   |                    */
	I_TARGET_DYNAMIC,     /* [ ] \  target flags     |                    */
	I_TARGET_TRANSIENT,   /* @   /                   | target word flags  */
	I_VARIABLE,           /* $                       |                    */
	I_NEWLINE_SEPARATED,  /* -n  \                   |                    */
	I_NUL_SEPARATED,      /* -0   | attribute flags  |                    */
	I_CODE,               /* -C  /                  /                     */
	I_INPUT,              /* <                                            */
	I_RESULT_NOTIFY,      /* -*                                           */
	I_RESULT_COPY,        /* -%                                           */

	C_ALL,
	C_PLACED                = 3,  /* Flags for which we store a place in Dep */
	C_WORD                  = 9,  /* Flags used for caching; stored in Target */
#define C_WORD                    9   /* Used statically */

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
	 * filenames, without any markup  */

	F_NUL_SEPARATED         = 1 << I_NUL_SEPARATED,
	/* For dynamic dependencies, the file contains NUL-separated
	 * filenames, without any markup  */

	F_CODE                  = 1 << I_CODE,
	/* For dynamic dependencies, the file contains Stu codde */

	F_INPUT                 = 1 << I_INPUT,
	/* A dependency is annotated with the input redirection flag '<' */

	F_RESULT_NOTIFY         = 1 << I_RESULT_NOTIFY,
	/* The link A ---> B between two executors annotated with this
	 * flags means that A is notified of B's results.  */

	F_RESULT_COPY           = 1 << I_RESULT_COPY,
	/* The link A ---> B between two executors annotated with this
	 * flags means that the results of B will be copied into A's result  */

	/*
	 * Aggregates
	 */
	F_PLACED        = (1 << C_PLACED) - 1,
	F_TARGET_BYTE   = (1 << C_WORD) - 1,
	F_TARGET        = F_TARGET_DYNAMIC | F_TARGET_TRANSIENT,
	F_ATTRIBUTE     = F_NEWLINE_SEPARATED | F_NUL_SEPARATED | F_CODE,
};

typedef unsigned Done;
/* Denotes which "aspects" of an execution have been done.  This is a different
 * way to encode the three placed flags.
 * The first two flags correspond to the first two flags (persistent and
 * optional).  These two are duplicated in order to accommodate trivial
 * dependencies.  */
enum
{
	D_NONPERSISTENT_TRANSIENT       = 1 << 0,
	D_NONOPTIONAL_TRANSIENT         = 1 << 1,
	D_NONPERSISTENT_NONTRANSIENT    = 1 << 2,
	D_NONOPTIONAL_NONTRANSIENT      = 1 << 3,

	D_ALL                           = (1 << 4) - 1,
	D_ALL_OPTIONAL                  = D_NONPERSISTENT_TRANSIENT | D_NONPERSISTENT_NONTRANSIENT,
};

extern const char *const flags_chars;
extern const char *flags_phrases[C_PLACED];

unsigned flag_get_index(char c);
/* Get the flag index corresponding to a character.  Not all cases are
 * implemented.  */

bool render_flags(Flags flags, Parts &, Rendering= 0);
string show_flags(Flags, Style= S_DEFAULT);

string format_done(Done done);

Done done_from_flags(Flags flags);
/* Only placed flags are kept */

#endif /* ! FLAGS_HH */
