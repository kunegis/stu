#ifndef FLAGS_HH
#define FLAGS_HH

/*
 * Flags apply to dependencies.  Flags are binary and option-like, and appear at multiple
 * levels in Stu, from Stu source code where they are represented by a syntax ressembling
 * that of command line options, to attributes of edges in the dependency graph.
 * Internally, flags are defined as bit fields.
 *
 * Each edge in the dependency graph is annotated with one object of this type.  This
 * contains bits related to what should be done with the dependency, whether time is
 * considered, etc.  The flags are defined in such a way that the simplest dependency is
 * represented by zero, and each flag enables a specific feature.
 *
 * The placed bits are effectively set for tasks not to do.  Therefore, inverting them
 * gives the bits for the tasks to do.  Other flags have the semantics of "more to do".
 */

typedef unsigned Flags;
typedef unsigned Index;

enum
{
	/* The index of the flags (I_*) */
	I_PERSISTENT= 0,      /* -p \   common flags    \                    */
	I_OPTIONAL,           /* -o /                    |                   */
	I_TRIVIAL,            /* -t                      |                   */
	I_TARGET_DYNAMIC,     /* [ ] \  target flags     |                   */
	I_TARGET_PHONY,       /* @   /                   | Hash_Dep          */
	I_VARIABLE,           /* $                       |                   */
	I_NEWLINE_SEPARATED,  /* -n  \                   |                   */
	I_NUL_SEPARATED,      /* -0   | dynamic format   |                   */
	I_CODE,               /* -C  /                   |                   */
	I_NO_FOLLOW,          /* -P                     /                    */
	I_INPUT,              /* <                                           */
	I_RESULT_NOTIFY,      /* -*                                          */
	I_RESULT_COPY,        /* -%                                          */
	I_PHASE_B,            /* -&                                          */

	/* Counts */
	C_ALL,
	C_COMMON_PLACED =  2, /* Flags that can appear in front of target or dependency */
	C_CACHE         =  9, /* Flags used for caching */
	C_WORD          = 10, /* Flags stored in Hash_Dep */

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

	F_TARGET_PHONY          = 1 << I_TARGET_PHONY,
	/* A phony target */

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

	F_NO_FOLLOW             = 1 << I_NO_FOLLOW,
	/* Target is symlink and should not be dereferenced */

	F_INPUT                 = 1 << I_INPUT,
	/* A dependency is annotated with the input redirection flag '<' */

	F_RESULT_NOTIFY         = 1 << I_RESULT_NOTIFY,
	/* The link A ---> B between two executors annotated with this flag
	 * means that A is notified of B's results. */

	F_RESULT_COPY           = 1 << I_RESULT_COPY,
	/* The link A ---> B between two executors annotated with this
	 * flag means that the results of B will be copied into A's result. */

	F_PHASE_B               = 1 << I_PHASE_B,
	/* A parent is in phase B */

	/* Aggregates */
	F_ALL           = (1 << C_ALL) - 1,
	F_COMMON_PLACED = (1 << C_COMMON_PLACED) - 1,
	F_CACHE         = (1 << C_CACHE) - 1,
	F_WORD          = (1 << C_WORD) - 1,
	F_ATTRIBUTE     = F_NEWLINE_SEPARATED | F_NUL_SEPARATED | F_CODE,
	F_RESULT        = F_RESULT_NOTIFY | F_RESULT_COPY,
	F_PLACED_TARGET= F_PERSISTENT | F_OPTIONAL | F_NO_FOLLOW,
	F_PLACED_DEPENDENCY= F_PERSISTENT | F_OPTIONAL | F_TRIVIAL
		| F_NEWLINE_SEPARATED | F_NUL_SEPARATED | F_CODE,
	F_PLACED= F_PLACED_TARGET | F_PLACED_DEPENDENCY,
	F_UNPLACED= F_ALL & ~F_PLACED,
};

constexpr const char flags_chars[]= "pot[@$n0CP<*%&";
static_assert(sizeof(flags_chars) == C_ALL + 1, "Keep in sync with Flags");
extern const char *flags_placed_phrases[C_ALL];

Index flag_get_index(char c); /* Must exist */
bool is_placed_flag_char(char c);

#endif /* ! FLAGS_HH */
