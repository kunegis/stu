#ifndef JOB_HH
#define JOB_HH

#include <map>
#include <string>

#include "error.hh"

class Job
/* A child process of Stu that executes the command for a given rule.  An object of this
 * type can execute a job only once. */
{
public:
	Job():  pid(-2) { }

	bool waited(int status, pid_t pid_check);
	/* Called after having returned this process from wait_do().  Return TRUE if the
	 * child was successful.  The PID is passed to verify that it is the correct
	 * one. */

	bool started() const  {  return pid >= 0;  }
	bool started_or_waited() const  {  return pid >= -1;  }

	/* Must be started */
	pid_t get_pid() const {
		assert(pid >= 0);
		return pid;
	}

	pid_t start(
		string command,
		const std::map <string, string> &mapping,
		string filename_output,
		string filename_input,
		const Place &place_command,
		const Place &place_output,
		const Place &place_input);
	/* Start the process.  Don't output the command -- this is done by callers of this
	 * function.  FILENAME_OUTPUT and FILENAME_INPUT are the files into which to
	 * redirect output and input; either can be empty to denote no redirection.  On
	 * error, output a message and return -1, otherwise return the PID (>= 0).
	 * MAPPING contains the environment variables to set. */

	pid_t start_copy(
		string target,
		string source,
		const Place &place);
	/* Start a copy job.  The return value has the same semantics as in start(). */

	static pid_t wait(int *status);
	/* Wait for the next process to terminate; provide the STATUS as
	 * used in wait(2).  Return the PID of the waited-for process (>=0). */

	static void print_statistics(bool allow_unterminated_jobs= false);
	/* Print the statistics about jobs, regardless of OPTION_STATISTICS.  If the
	 * argument is set, there must not be unterminated jobs. */

	static void kill(pid_t pid);
	static int get_fd_tty(); /* -1 if there is none */

private:
	pid_t pid;
	/*
	 * -2:    process was not yet started.
	 * >= 0:  process was started but not yet waited for (just called "started" for
	 *        short).  It may already be finished, i.e., a zombie.
	 * -1:    process has been waited for.
	 */

	static size_t count_jobs_exec, count_jobs_success, count_jobs_fail;
	/* The number of jobs run.  Each job is of exactly one type.
	 *
	 * Exec:     Currently being executed
	 * Success:  Finished, with success
	 * Fail:     Finished, without success */

	static pid_t pid_foreground;
	/* The job that is in the foreground, or -1 when none is */

	static void ask_continue(pid_t pid);
	static const char **create_child_env(const std::map <string, string> &mapping);
	static const char **create_child_argv(
		const Place &place_command,
		const char *shell_shortname,
		string &command,
		string &argv0);
	static const char *get_shell(const char *&shell_shortname);
	static const char *get_cp(const char *&cp_shortname);
	static const char *get_shortname(const char *name);
	static void create_child_output_redirection(string filename_output, const Place &);
	static void create_child_input_redirection(string filename_input, const Place &);
};

#endif /* ! JOB_HH */
