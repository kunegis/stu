#ifndef PROCEED_HH
#define PROCEED_HH

/*
 * This is used as the return value of the functions Executor::execute*().
 */

typedef unsigned Proceed;

enum {
	P_WAIT       = 1 << 0,
	/* There's more to do, but it can only be started after having waited
	 * for other jobs to finish.  I.e., the wait function will have to be
	 * called. */

	P_CALL_AGAIN = 1 << 1,
	/* The function execute() should be called again for this executor
	 * (without waiting) at least, for various reasons, mostly for
	 * randomization of execution order. */

	P_FINISHED   = 1 << 2,
	/* This Executor is finished. */

	P_ABORT      = 1 << 3,
	/* This Executor should be finished immediately.  When set, P_FINISHED
	 * is also set.  This does not imply that there was an error -- for
	 * instance, the trivial flag -t may mean that nothing more should be
	 * done. */

	P_COUNT      = 4
};

string show_proceed(Proceed proceed);

#endif /* ! PROCEED_HH */
