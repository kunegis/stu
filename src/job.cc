#include "job.hh"

#include <sys/resource.h>

#include "file_executor.hh"

#ifdef STU_COV
extern "C" {
#include "gcov.h"
}
#else
#define __gcov_dump()
#endif

size_t Job::count_jobs_exec=    0;
size_t Job::count_jobs_success= 0;
size_t Job::count_jobs_fail=    0;
sigset_t Job::set_termination;
sigset_t Job::set_productive;
sigset_t Job::set_termination_productive;
volatile sig_atomic_t Job::in_child= 0;
pid_t Job::pid_foreground= -1;
int Job::tty= -1;
bool Job::signals_initialized;

#ifndef NDEBUG
bool Job::Signal_Blocker::blocked= false;
#endif

pid_t Job::start(string command,
		 const map <string, string> &mapping,
		 string filename_output,
		 string filename_input,
		 const Place &place_command)
{
	assert(pid == -2);
	init_signals();

	/* Like Make, we don't use the variable $SHELL, but use "/bin/sh" as a
	 * shell instead.  The reason is that the variable $SHELL is intended to
	 * denote the user's chosen interactive shell, and may not be a
	 * POSIX-compatible shell.  Note also that POSIX prescribes that Make
	 * use "/bin/sh" by default.  Other note:  Make allows to declare the
	 * Make variable $SHELL within the Makefile or in Make's parameters to a
	 * value that *will* be used by Make instead of /bin/sh.  This is not
	 * possible with Stu, because Stu does not have its own set of
	 * variables.  Instead, there is the $STU_SHELL variable.  The
	 * Stu-native way to do it without environment variables would be via a
	 * directive (but such a directive is not implemented).  */
	static const char *shell= nullptr;
	if (shell == nullptr) {
		shell= getenv("STU_SHELL");
		if (shell == nullptr || shell[0] == '\0')
			shell= "/bin/sh";
	}

	const char *arg= command.c_str();
	/* c_str() never returns nullptr, as by the standard */
	assert(arg != nullptr);

	pid= fork();

	if (pid < 0) {
		print_errno("fork");
		assert(pid == -1);
		return -1;
	}

	/* Each child process is given, as process group ID, its process
	 * ID.  This ensures that we can kill each child by killing its
	 * corresponding process group ID.  How process groups work:  Each
	 * process has not only a process ID (PID), but also a process group ID
	 * (PGID).  The PGID is inherited by child processes, without being
	 * changed automatically.  This means that if the PGID is never changed
	 * by a process, a process and all its child processes will have the
	 * same PGID.  This makes it possible to kill a process and all its
	 * children (directly and indirectly) by passing the negated PGID as the
	 * first parameter to kill(2).  Thus, we set the child process to have
	 * as its PGID the same value as its PID.  */

	/* Execute this in both the child and parent */
	const int pid_child= pid == 0 ? getpid() : pid;
	if (0 > setpgid(pid_child, pid_child)) {
		/* This should only fail when we are the parent and the
		 * child has already quit.  In that case we can ignore
		 * the error, since the child is dead anyway, so there
		 * is no need to kill it in the future.  */
	}

	if (pid == 0) {
		/* We are the child process */
		in_child= 1;

		/* Instead of throwing exceptions, use perror() and
		 * _Exit(ERROR_FORK_CHILD).  */

		/* Unblock/reset all signals.  As a general rule,
		 * signals that are blocked before exec() will remain
		 * blocked after exec().  Thus, unblock them here.  */
		if (0 != sigprocmask(SIG_UNBLOCK, &set_termination_productive, nullptr)) {
			perror("sigprocmask");
			__gcov_dump();
			_Exit(ERROR_FORK_CHILD);
		}
		::signal(SIGTTIN, SIG_DFL);
		::signal(SIGTTOU, SIG_DFL);

		/* Set variables */
		size_t v_old= 0;

		map <string, size_t> old;
		/* Index of old variables */

		while (envp_global[v_old]) {
			const char *p= envp_global[v_old];
			const char *q= p;
			while (*q && *q != '=')  ++q;
			string key_old(p, q-p);
			old[key_old]= v_old;
			++v_old;
		}

		const size_t v_new= mapping.size() + 1;
		/* Maximal size of added variables.  The "+1" is for $STU_STATUS */

		const char** envp= (const char **)
			malloc(sizeof(char **) * (v_old + v_new + 1));
		if (!envp) {
			perror("malloc");
			__gcov_dump();
			_Exit(ERROR_FORK_CHILD);
		}
		memcpy(envp, envp_global, v_old * sizeof(char **));
		size_t i= v_old;
		for (auto j= mapping.begin(); j != mapping.end(); ++j) {
			string key= j->first;
			string value= j->second;
			assert(key.find('=') == string::npos);
			size_t len_combined= key.size() + 1 + value.size() + 1;
			char *combined= (char *)malloc(len_combined);
			if (! combined) {
				perror("malloc");
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			if ((ssize_t)(len_combined - 1) != snprintf(combined, len_combined, "%s=%s", key.c_str(), value.c_str())) {
				perror("snprintf");
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			if (old.count(key)) {
				size_t v_index= old.at(key);
				envp[v_index]= combined;
			} else {
				assert(i < v_old + v_new);
				envp[i++]= combined;
			}
		}
		envp[i++]= "STU_STATUS=1";
		assert(i <= v_old + v_new);
		envp[i]= nullptr;

		/* As $0 of the process, we pass the filename of the
		 * command followed by a colon, the line number, a colon
		 * and the column number.  This makes the shell if it
		 * reports an error make the most useful output.  */
		string argv0= place_command.as_argv0();
		if (argv0.empty())
			argv0= shell;

		/* The one-character options to the shell */
		/* We use the -e option ('error'), which makes the shell abort
		 * on a command that fails.  This is also what POSIX prescribes
		 * for Make.  It is particularly important for Stu, as Stu
		 * invokes the whole (possibly multiline) command in one step. */
		const char *shell_options= option_x ? "-ex" : "-e";

		const char *argv[]= {argv0.c_str(),
				     shell_options, "-c", arg, nullptr};

		/*
		 * Special handling of the case when the command
		 * starts with '-' or '+'.  In that case, we prepend
		 * a space to the command.  We cannot use '--' as
		 * prescribed by POSIX because Linux and FreeBSD handle
		 * '--' differently:
		 *
		 *      /bin/sh -c -- '+x'
		 *      on Linux: Execute the command '+x'
		 *      on FreeBSD: Execute the command '--' and set
		 *                  the +x option
		 *
		 *      /bin/sh -c +x
		 *      on Linux: Set the +x option, and missing
		 *                argument to -c
		 *      on FreeBSD: Execute the command '+x'
		 *
		 * See:
		 * http://stackoverflow.com/questions/37886661/handling-of-in-arguments-of-bin-sh-posix-vs-implementations-by-bash-dash
		 *
		 * It seems that FreeBSD violates POSIX in this regard.
		 */

		if (arg[0] == '-' || arg[0] == '+') {
			command= ' ' + command;
			arg= command.c_str();
			argv[3]= arg;
		}

		/* Output redirection */
		if (!filename_output.empty()) {
			int fd_output= creat
				(filename_output.c_str(),
				 /* All +rw, i.e. 0666 */
				 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if (fd_output < 0) {
				perror(filename_output.c_str());
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			assert(fd_output != 1);
			int r= dup2(fd_output, 1); /* 1 = file descriptor of STDOUT */
			if (r < 0) {
				perror(filename_output.c_str());
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			assert(r == 1);
			close(fd_output);
		}

		/* Input redirection:  from the given file, or from
		 * /dev/null (in non-interactive mode)  */
		if (!filename_input.empty() || ! option_i) {
			const char *name= filename_input.empty()
				? "/dev/null"
				: filename_input.c_str();
			int fd_input= open(name, O_RDONLY);
			if (fd_input < 0) {
				perror(name);
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			assert(fd_input >= 3);
			int r= dup2(fd_input, 0); /* 0 = file descriptor of STDIN */
			if (r < 0) {
				perror(name);
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
			assert(r == 0);
			if (close(fd_input) < 0) {
				perror(name);
				__gcov_dump();
				_Exit(ERROR_FORK_CHILD);
			}
		}

		__gcov_dump();
		int r= execve(shell, (char *const *) argv, (char *const *) envp);
		assert(r == -1);
		perror("execve");
		__gcov_dump();
		_Exit(ERROR_FORK_CHILD);
	}

	/* We are the parent process */
	assert(pid >= 1);
	if (option_i && tty >= 0) {
		assert(pid_foreground < 0);
		if (tcsetpgrp(tty, pid) < 0)
			print_errno("tcsetpgrp");
		pid_foreground= pid;
	}
	++ count_jobs_exec;
	return pid;
}

/* This function works analogously to start() with respect to invocation
 * of fork() and other system-related functions.  */
pid_t Job::start_copy(string target,
		      string source)
{
	assert(! target.empty());
	assert(! source.empty());
	assert(pid == -2);

	init_signals();

	pid= fork();

	if (pid < 0) {
		print_errno("fork");
		assert(pid == -1);
		return -1;
	}

	int pid_child= pid;
	if (pid_child == 0)
		pid_child= getpid();
	if (0 > setpgid(pid_child, pid_child)) {
		/* no-op */
	}

	if (pid == 0) {
		/* We are the child process */

		/* We don't set $STU_STATUS for copy jobs */

		static const char *cp_command= nullptr;
		if (cp_command == nullptr) {
			cp_command= getenv("STU_CP");
			if (cp_command == nullptr || cp_command[0] == '\0')
				cp_command= "/bin/cp";
		}

		/* Using '--' as an argument guarantees that the two
		 * filenames will be interpreted as filenames and not as
		 * options, in particular when they begin with a dash.  */
		const char *argv[]= {cp_command,
				     "--",
				     source.c_str(),
				     target.c_str(),
				     nullptr};
		__gcov_dump();
		int r= execv(cp_command, (char *const *) argv);
		assert(r == -1);
		perror("execv");
		__gcov_dump();
		_Exit(ERROR_FORK_CHILD);
	}

	/* Parent execution */
	++ count_jobs_exec;
	assert(pid >= 1);
	return pid;
}

pid_t Job::wait(int *status)
/* The main loop of Stu.  We wait for the two productive signals SIGCHLD
 * and SIGUSR1.  When this function is called, there is always at least
 * one child process running.  */
{
 begin:
	/* First, try wait() without blocking.  WUNTRACED is used to also get
	 * notified when a job is suspended (e.g. with Ctrl-Z).  */
	pid_t pid= waitpid(-1, status,
			   WNOHANG | (option_i ? WUNTRACED : 0));
	if (pid < 0) {
		/* Should not happen as there is always something running when
		 * this function is called.  However, this may be common enough
		 * that we may want Stu to act correctly.  */
		should_not_happen();
		perror("waitpid");
		abort();
	}

	if (pid > 0) {
		if (WIFSTOPPED(*status)) {
			/* The process was suspended. This can have
			 * several reasons, including someone just using
			 * kill -STOP on the process.  */
			assert(option_i);

			/* This is the simplest thing possible we can do
			 * in interactive mode: put ourselves in the
			 * foreground, ask the user to press ENTER, and
			 * then put the job back into the foreground and
			 * continue it.  In principle, we could do much
			 * more: allow the user to enter commands,
			 * having an own command language, etc.  */
			if (tcsetpgrp(tty, getpid()) < 0)
				print_errno("tcsetpgrp");
			fprintf(stderr,
				PACKAGE ": job stopped.  "
				"Press ENTER to continue, Ctrl-C to terminate Stu, Ctrl-Z to suspend Stu\n");
			char *lineptr= nullptr;
			size_t n= 0;
			ssize_t r= getline(&lineptr, &n, stdin);
			/* On error, printf error message and continue */
			if (r < 0)
				print_errno("getline");
			fprintf(stderr, PACKAGE ": continuing\n");
			if (tcsetpgrp(tty, pid) < 0)
				print_errno("tcsetpgrp");
			/* Continue job */
			::kill(-pid, SIGCONT);
			goto begin;
		}
		return pid;
	}

	/* Any SIGCHLD sent after the last call to sigwait() will be
	 * ready for receiving, even those SIGCHLD signals received
	 * between the last call to waitpid() and the following call to
	 * sigwait().  This excludes a deadlock which would be
	 * possible if we would only use sigwait(). */

	int sig;
	int r;
 retry:
	/* We block the termination signals and wait for them using sigwait(),
	 * because the signal handlers for them may not be called while
	 * sigwait() is pending.  Depending on the system, sigwait() may allow
	 * handlers for non-blocked signals to be executed while sigwait()
	 * waits, or have them executed only once sigwait() returns.  Note that
	 * sigwaitinfo() should (in principle) not have this problem, but it is
	 * less portable. */
	{
		Signal_Blocker signal_blocker;
		errno= 0;
		r= sigwait(&set_termination_productive, &sig);
	}

	if (r != 0) {
		if (errno == EINTR) {
			/* This should not happen, but be prepared */
			should_not_happen();
			goto retry;
		} else {
			perror("sigwait");
			abort();
		}
	}

	int is_termination= sigismember(&set_termination, sig);
	if (is_termination == 1) {
		/* If a termination signal arrives while we were waiting, the
		 * signal handler may be called immediately after sigwait()
		 * ends, or it may not. */
		raise(sig);
		perror("raise");
		exit(ERROR_FATAL);
	} else if (is_termination != 0) {
		perror("sigismember");
		exit(ERROR_FATAL);
	}

	switch (sig) {
	case SIGCHLD:
		/* Don't act on the signal here.  We could get the PID and
		 * STATUS from siginfo, but then the process would stay a
		 * zombie.  Therefore, we have to call waitpid().  The call to
		 * waitpid() will then return the proper signal. */
		goto begin;
	case SIGUSR1:
		print_statistics(true);
		print_jobs();
		goto retry;
	default:
		/* We didn't wait for this signal */
		should_not_happen();
		print_error(frmt("sigwait: received signal %d", sig));
		goto begin;
	}
}

bool Job::waited(int status, pid_t pid_check)
{
	assert(pid_check >= 0);
	assert(pid >= 0);
	assert(pid_check == pid);

	bool success= WIFEXITED(status) && WEXITSTATUS(status) == 0;

	if (success)
		++ count_jobs_success;
	else
		++ count_jobs_fail;

	if (pid == pid_foreground) {
		assert(tty >= 0);
		assert(option_i);
		if (tcsetpgrp(tty, getpid()) < 0)
			print_errno("tcsetpgrp");
	}
	pid= -1;
	return success;
}

void Job::print_statistics(bool allow_unterminated_jobs)
{
	/* Avoid double writing in case the destructor gets still called */
	option_z= false;
	struct rusage usage;

	int r= getrusage(RUSAGE_CHILDREN, &usage);
	if (r < 0) {
		print_errno("getrusage");
		throw ERROR_BUILD;
	}

	if (! allow_unterminated_jobs)
		assert(count_jobs_exec == count_jobs_success + count_jobs_fail);
	assert(count_jobs_exec >= count_jobs_success + count_jobs_fail);

	if (! allow_unterminated_jobs)
		printf("STATISTICS  number of jobs started = %zu "
		       "(%zu succeeded, %zu failed)\n",
		       count_jobs_exec, count_jobs_success, count_jobs_fail);
	else
		printf("STATISTICS  number of jobs started = %zu "
		       "(%zu succeeded, %zu failed, %zu running)\n",
		       count_jobs_exec, count_jobs_success, count_jobs_fail,
		       count_jobs_exec - count_jobs_success - count_jobs_fail);

	printf("STATISTICS  children user   execution time = %ju.%06lu s\n",
	       (uintmax_t)     usage.ru_utime.tv_sec,
	       (unsigned long) usage.ru_utime.tv_usec);
	printf("STATISTICS  children system execution time = %ju.%06lu s\n",
	       (uintmax_t)     usage.ru_stime.tv_sec,
	       (unsigned long) usage.ru_stime.tv_usec);
	printf("STATISTICS  Note: children execution times exclude running jobs\n");
}

void Job::handler_termination(int sig)
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
	if (Job::in_child == 0) {
		/* Terminate all processes */
		terminate_jobs();
	} else {
		assert_async(Job::in_child == 1);
	}

	/* We cannot call Job::Statistics::print() here because
	 * getrusage() is not async signal safe, and because the count_*
	 * variables are not atomic.  Not even functions like fputs()
	 * are async signal-safe, so don't even try.  */

	/* Raise signal again */
	int rr= raise(sig);
	if (rr != 0) {
		write_async(2, "stu: error: raise\n");
	}

	/* Don't abort here -- the reraising of this signal may only be
	 * delivered after this handler is done. */
	errno= errno_save;
}

void Job::handler_productive(int, siginfo_t *, void *)
/* Do nothing -- the handler only exists because POSIX says that a
 * signal may be discarded by the kernel if there is no signal handler
 * for it, and then it may not be possible to sigwait() for that signal.  */
{
	/* [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions here */
}

Job::Signal_Blocker::Signal_Blocker()
{
#ifndef NDEBUG
	assert(!blocked);
	blocked= true;
#endif
	if (0 != sigprocmask(SIG_BLOCK, &set_termination, nullptr)) {
		perror("sigprocmask");
		exit(ERROR_FATAL);
	}
}

Job::Signal_Blocker::~Signal_Blocker()
{
#ifndef NDEBUG
	assert(blocked);
	blocked= false;
#endif
	if (0 != sigprocmask(SIG_UNBLOCK, &set_termination, nullptr)) {
		perror("sigprocmask");
		exit(ERROR_FATAL);
	}
}

void Job::init_signals()
/*
 * There are three types of signals handled by Stu:
 *    - Termination signals which make programs abort.  Stu catches
 *      them in order to stop its child processes and remove temporary
 *      files, and will then raise them again.  The handlers for these
 *      are async-signal safe.
 *    - Productive signals that actually inform the Stu process of
 *      something:
 *         + SIGCHLD (to know when child processes are done)
 *         + SIGUSR1 (to output statistics)
 *      These signals are blocked, and then waited for specifically.
 *      The handlers thus do not have to be async-signal safe.
 *    - The job control signals SIGTTIN and SIGTTOU.  They are both
 *      produced by certain job control events that Stu triggers, and
 *      ignored by Stu.
 *
 * The signals SIGCHLD and SIGUSR1 are the signals that we wait for in
 * the main loop.  They are blocked.  At the same time, each blocked
 * signal must have a signal handler (which can do nothing), as
 * otherwise POSIX allows the signal to be discarded.  Thus, we setup a
 * no-op signal handler.  (Note that Linux does not discard such
 * signals, while FreeBSD does.)
 */
{
	if (signals_initialized)
		return;
	signals_initialized= true;

	if (0 != sigemptyset(&set_termination_productive))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}

	/*
	 * Termination signals
	 */

	struct sigaction act_termination;
	act_termination.sa_handler= handler_termination;
	if (0 != sigemptyset(&act_termination.sa_mask))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	act_termination.sa_flags= 0;
	if (0 != sigemptyset(&set_termination))  {
		perror("sigemptyset");
		exit(ERROR_FATAL);
	}
	/* These are all signals that by default would terminate the
	 * process.  */
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

	/* We have to use sigaction() rather than signal() as only
	 * sigaction() guarantees that the signal can be queued, as per
	 * POSIX.  */
	struct sigaction act_productive;
	act_productive.sa_sigaction= Job::handler_productive;
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

void Job::kill(pid_t pid)
/* Passing (-pid) to kill() kills the whole process group with PGID
 * (pid).  Since we set each child process to have its PID as its
 * process group ID, this kills the child and all its children
 * (recursively), up to programs that change this PGID of processes,
 * such as Stu and shells, which have to kill their children explicitly
 * in their signal handlers.  */
{
	/* [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions here */

	assert_async(pid > 1);

	/* We send first SIGTERM, then SIGCONT */
	if (0 > ::kill(-pid, SIGTERM)) {
		if (errno == ESRCH) {
			/* The child process is a zombie.  This
			 * means the child process has already
			 * terminated but we haven't wait()ed
			 * for it yet.  */
		} else {
			write_async(2, "stu: error: kill\n");
			/* Note:  Don't call exit() yet; we want all
			 * children to be killed.  */
		}
	}

	if (0 > ::kill(-pid, SIGCONT)) {
		if (errno != ESRCH) {
			write_async(2, "stu: error: kill\n");
		}
	}
}

void Job::init_tty()
{
	assert(tty == -1);
	tty= open("/dev/tty", O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (tty < 0) {
		assert(tty == -1);
		return;
	}
}
