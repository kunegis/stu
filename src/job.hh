#ifndef JOB_HH
#define JOB_HH

/*
 * Handling of child processes, including signal-related issues.
 */

#include <signal.h>
#include <map>
#include <string>

#include "error.hh"

// TODO move the declaration of the following two functions to where they are
// implemented. 

void job_terminate_all();
/* Called to terminate all running processes, and remove their target
 * files if present.  Implemented in file_executor.cc, and called from
 * here. */

void job_print_jobs();
/* Called from here; implemented in file_execution.cc */

/*
 * Macro to write in an async signal-safe manner.
 *   - FD must be '1' or '2'.
 *   - MESSAGE must be a string literal.
 * Ignore errors, as this is called from the terminating signal handler.
 *
 * [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions in this
 * macro.  
 *
 * Note:  This macro may change ERRNO.
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
 * [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions in this
 * macro.  
 * Note:  This macro may change ERRNO.
 */
#ifdef NDEBUG
#	define assert_async(X)  ((void) 0 )
#else /* ! NDEBUG */
#	define assert_async(X)  ((void)( (X) || (write(2, "assert_async failed\n", 20), abort(), 0)))
#endif /* ! NDEBUG */

class Job
/* A child process of Stu that executes the command for a given rule.  An object
 * of this type can execute a job only once.  */
{
public:
	Job():  pid(-2) { }

	bool waited(int status, pid_t pid_check);
	/* Called after having returned this process from wait_do().
	 * Return TRUE if the child was successful.  The PID is passed
	 * to verify that it is the correct one.  */

	bool started() const  {  return pid >= 0;  }
	bool started_or_waited() const  {  return pid >= -1;  }

	/* Must be started */
	pid_t get_pid() const {
		assert(pid >= 0);
		return pid;
	}

	pid_t start(string command,
		    const map <string, string> &mapping,
		    string filename_output,
		    string filename_input,
		    const Place &place_command);
	/* Start the process.  Don't output the command -- this is done
	 * by callers of this functions.  FILENAME_OUTPUT and
	 * FILENAME_INPUT are the files into which to redirect output
	 * and input; either can be empty to denote no redirection.  On
	 * error, output a message and return -1, otherwise return the
	 * PID (>= 0).  MAPPING contains the environment variables to
	 * set.  */

	pid_t start_copy(string target, string source);
	/* Start a copy job.  The return value has the same semantics as
	 * in start().  */

	static pid_t wait(int *status);
	/* Wait for the next process to terminate; provide the STATUS as
	 * used in wait(2).  Return the PID of the waited-for process (>=0). */

	static void print_statistics(bool allow_unterminated_jobs= false);
	/* Print the statistics about jobs, regardless of OPTION_STATISTICS.  If
	 * the argument is set, there must not be unterminated jobs.  */

	static void kill(pid_t pid);
	/* Kill this job */

	static void init_tty();
	static pid_t get_tty()  {  return tty;  }

	class Signal_Blocker
	/* Block termination signals for the lifetime of an object of this type.
	 * Note that the mask of blocked signals is inherited over exec(), so we
	 * must unblock signals also when starting child processes.  */
	{
	private:
#ifndef NDEBUG
		static bool blocked;
#endif
	public:
		Signal_Blocker();
		~Signal_Blocker();
	};

private:
	pid_t pid;
	/*
	 * -2:    process was not yet started.
	 * >= 0:  process was started but not yet waited for (just called
	 *        "started" for short).  It may already be finished,
	 * 	  i.e., a zombie.
	 * -1:    process has been waited for.
	 */

	/* The signal handlers */
	static void handler_termination(int sig);
	static void handler_productive(int sig, siginfo_t *, void *);

	static void init_signals();
	/* Set up all signals.   May be called multiple times, and will
	 * do the setup only the first time.  */

	static size_t count_jobs_exec, count_jobs_success, count_jobs_fail;
	/* The number of jobs run.  Each job is of exactly one type.
	 *
	 * Exec:     Currently being executed
	 * Success:  Finished, with success
	 * Fail:     Finished, without success  */

	static sigset_t set_termination, set_productive,
		set_termination_productive;
	/* All signals handled specially by Stu are either in the
	 * "termination" or in the "productive" set.  The third variable
	 * holds both.  */

	static volatile sig_atomic_t in_child;
	/* Set to 1 in the child process, before execve() is called; 0 in the
	 * parent process.  Used to avoid doing too much in the terminating
	 * signal handler.  Note: There is a race condition because the signal
	 * handler may be called before the variable is set.  This is the only
	 * variable in Stu that is "volatile sig_atomic_t".  */

	static pid_t pid_foreground;
	/* The job that is in the foreground, or -1 when none is */

	static int tty;
	/* The file descriptor of the TTY used by Stu.  -1 if there is none. */

	static bool signals_initialized;
};

#endif /* ! JOB_HH */
