#include "signal.hh"

#ifndef NDEBUG
bool Signal_Blocker::blocked= false;
#endif

sigset_t set_termination, set_productive, set_termination_productive;
volatile sig_atomic_t in_child= 0;

Signal_Blocker::Signal_Blocker()
{
#ifndef NDEBUG
	assert(!blocked);
	blocked= true;
#endif
	if (0 != sigprocmask(SIG_BLOCK, &set_termination, nullptr)) {
		perror("sigprocmask");
		abort();
	}
}

Signal_Blocker::~Signal_Blocker()
{
#ifndef NDEBUG
	assert(blocked);
	blocked= false;
#endif
	if (0 != sigprocmask(SIG_UNBLOCK, &set_termination, nullptr)) {
		perror("sigprocmask");
		abort();
	}
}

void signal_handler_termination(int sig)
/*
 * Terminate all jobs and quit.
 */
{
	/* [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions here */

	int errno_save= errno;

	/* Reset the signal to its default action */
	struct sigaction act;
	act.sa_handler= SIG_DFL;
	if (0 != sigemptyset(&act.sa_mask))  {
		write_async(2, "stu: error: sigemptyset\n");
	}
	act.sa_flags= SA_NODEFER;
	int r= sigaction(sig, &act, nullptr);
	assert_async(r == 0);

	/* If in the child process (the short time between fork() and
	 * exec()), just quit */
	if (in_child == 0) {
		terminate_jobs();
	} else {
		assert_async(in_child == 1);
	}

	/* We cannot call Job::Statistics::print() here because getrusage() is not async
	 * signal safe, and because the count_* variables are not atomic.  Not even
	 * functions like fputs() are async signal-safe, so don't even try. */

	/* Raise signal again */
	__gcov_dump();
	int rr= raise(sig);
	if (rr != 0) {
		write_async(2, "stu: error: raise\n");
	}

	/* Don't abort here -- the reraising of this signal may only be
	 * delivered after this handler is done. */
	errno= errno_save;
	__gcov_dump();
}

void signal_handler_productive(int, siginfo_t *, void *) /* uncovered */
/* Do nothing -- the handler only exists because POSIX says that a signal may be discarded
 * by the kernel if there is no signal handler for it, and then it may not be possible to
 * sigwait() for that signal. */
/* [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions here */
{
	/* uncovered */
}

void init_signals()
{
	static bool signals_initialized = false;
	if (signals_initialized) return;
	signals_initialized= true;

	if (0 != sigemptyset(&set_termination_productive))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}

	/*
	 * Termination signals
	 */

	struct sigaction act_termination;
	act_termination.sa_handler= signal_handler_termination;
	if (0 != sigemptyset(&act_termination.sa_mask))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	act_termination.sa_flags= 0;
	if (0 != sigemptyset(&set_termination))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	/* These are all signals that by default would terminate the process. */
	const static int signals_termination[]= {
		SIGTERM, SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGPIPE,
		SIGILL, SIGHUP,
	};

	for (size_t i= 0;
	     i < sizeof(signals_termination) / sizeof(signals_termination[0]); ++i) {
		if (0 != sigaction(signals_termination[i], &act_termination, nullptr)) {
			perror("sigaction");
			exit(ERROR_FATAL);
		}
		if (0 != sigaddset(&set_termination, signals_termination[i])) {
			perror("sigaddset");
			exit(ERROR_FATAL);
		}
		if (0 != sigaddset(&set_termination_productive, signals_termination[i])) {
			perror("sigaddset");
			exit(ERROR_FATAL);
		}
	}

	/*
	 * Productive signals
	 */

	/* We have to use sigaction() rather than signal() as only sigaction() guarantees
	 * that the signal can be queued, as per POSIX. */
	struct sigaction act_productive;
	act_productive.sa_sigaction= signal_handler_productive;
	if (sigemptyset(& act_productive.sa_mask)) {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	act_productive.sa_flags= SA_SIGINFO;
	sigaction(SIGCHLD, &act_productive, nullptr);
	sigaction(SIGUSR1, &act_productive, nullptr);

	if (0 != sigemptyset(&set_productive)) {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	if (0 != sigaddset(&set_productive, SIGCHLD)) {
		perror("sigaddset");
		exit(ERROR_FATAL);
	}
	if (0 != sigaddset(&set_productive, SIGUSR1)) {
		perror("sigaddset");
		exit(ERROR_FATAL);
	}
	if (0 != sigaddset(&set_termination_productive, SIGCHLD)) {
		perror("sigaddset");
		exit(ERROR_FATAL);
	}
	if (0 != sigaddset(&set_termination_productive, SIGUSR1)) {
		perror("sigaddset");
		exit(ERROR_FATAL);
	}
	if (0 != sigprocmask(SIG_BLOCK, &set_productive, nullptr)) {
		perror("sigprocmask");
		exit(ERROR_FATAL);
	}

	/*
	 * Job control signals
	 */
	if (::signal(SIGTTIN, SIG_IGN) == SIG_ERR)
		print_errno("signal");

	if (::signal(SIGTTOU, SIG_IGN) == SIG_ERR)
		print_errno("signal");
}
