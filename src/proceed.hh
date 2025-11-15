#ifndef PROCEED_HH
#define PROCEED_HH

/*
 * The return value of the functions Executor::execute*().
 */

typedef unsigned Proceed;

enum {
	P_WAIT       = 1 << 0,
	/* There's more to do, but it can only be started after having waited for other
	 * jobs to finish.  I.e., the wait function will have to be called. */

	P_CALL_AGAIN = 1 << 1,
	/* The function execute() should be called again for this executor (without
	 * waiting) at least, for various reasons, mostly for randomization of execution
	 * order. */

	P_COUNT      = 2,
	P_NOTHING    = 0,
};

#ifndef NDEBUG
bool is_valid(Proceed proceed);
string show(Proceed proceed);
#endif /* ! NDEBUG */

#endif /* ! PROCEED_HH */
