#ifndef SIGNAL_HH
#define SIGNAL_HH

/*
 * There are three types of signals handled by Stu:
 *    - Termination signals which make programs abort.  Stu catches them in order to stop
 *      its child processes and remove temporary files, and will then raise them again.
 *      The handlers for these are async-signal safe.
 *    - Productive signals that actually inform the Stu process of something:
 *         + SIGCHLD (to know when child processes are done)
 *         + SIGUSR1 (to output statistics)
 *      These signals are blocked, and then waited for specifically.  The handlers thus do
 *      not have to be async-signal safe.
 *    - The job control signals SIGTTIN and SIGTTOU.  They are both produced by certain
 *      job control events that Stu triggers, and ignored by Stu.
 *
 * The signals SIGCHLD and SIGUSR1 are the signals that we wait for in the main loop.
 * They are blocked.  At the same time, each blocked signal must have a signal handler
 * (which can do nothing), as otherwise POSIX allows the signal to be discarded.  Thus, we
 * setup a no-op signal handler.  (Linux does not discard such signals, while FreeBSD
 * does.)
 */

/*
 * Write in an async signal-safe manner.  FD must be '1' or '2'.  MESSAGE must be a string
 * literal.  Ignore errors, as this is called from the terminating signal handler.
 *
 * [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions in this
 * macro.
 *
 * This macro may change ERRNO.
 */
#define write_async(FD, MESSAGE) \
	do { \
		ssize_t r_write_safe= write(FD, MESSAGE, sizeof(MESSAGE) - 1); \
		(void)r_write_safe; \
	} while(0)

/*
 * The same as assert(), but in an async-signal safe way.  In all
 * likelihood, implementations of assert() are already async-signal
 * safe, but we shouldn't rely on that.
 *
 * [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions in this macro.
 *
 * Note:  This macro may change ERRNO.
 */
#ifdef NDEBUG
#	define assert_async(X)  ((void) 0 )
#else /* ! NDEBUG */
#	define assert_async(X)  ((void)( (X) || (write(2, "assert_async failed\n", 20), abort(), 0)))
#endif /* ! NDEBUG */

extern sigset_t set_termination, set_productive, set_termination_productive;

extern volatile sig_atomic_t in_child;
/* Set to 1 in the child process, before execve() is called; 0 in the parent process.
 * Used to avoid doing too much in the terminating signal handler.  Note: There is a race
 * condition because the signal handler may be called before the variable is set. */

class Signal_Blocker
/* Block termination signals for the lifetime of an object of this type.  The mask of
 * blocked signals is inherited over exec(), so we must unblock signals also when starting
 * child processes. */
{
public:
	Signal_Blocker();
	~Signal_Blocker();
#ifndef NDEBUG
	static bool is_blocked() { return blocked; }
#endif
private:
#ifndef NDEBUG
	static bool blocked;
#endif
};

void signal_handler_termination(int sig);
void signal_handler_productive(int sig, siginfo_t *, void *);
void init_signals();

#endif /* ! SIGNAL_HH */
