#ifndef JOB_HH
#define JOB_HH

/* Handling of child processes, inlcuding signal-related issues.  
 */

#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <unordered_map>

/* Called to terminate all running processes, and remove their target
 * files if present */
void job_terminate_all(); 

void job_print_jobs(); 

/* Write in an async signal-safe manner. 
 * 	FD must be 1 or 2.
 * 	MESSAGE must be a string literal. 
 * Ignore errors, as this is called into from the interrupting signal handler. 
 */
#define write_safe(FD, MESSAGE) do {int r_write_safe= write(FD, MESSAGE, sizeof(MESSAGE) - 1); (void)r_write_safe;}while(0)

class Job
{
private:
	/* -2:    process was not yet started.
	 * >= 0:  process was started but not yet waited for (just called
	 *        "started" for short. 
	 * -1:    process has been waited for. 
	 */
	pid_t pid;

	static void handler_interrupt(int sig);

	/* The number of jobs run.  
	 * exec:  executed
	 * success:  returned successfully
	 * fail:     returned as failing
	 */
	static unsigned count_jobs_exec, count_jobs_success, count_jobs_fail;

	/* Initialized to all signals that are blocked and then waited
	 * for */ 
	static sigset_t set_block;

	/* All signals that interrupt Stu. */ 
	static sigset_t set_interrupt;

	static sig_atomic_t in_child; 

	static class Statistics
	{
	public:
		/* Run after main() */ 
		~Statistics() 
		{
			if (! option_statistics)  return; 
			print(); 
		}

		/* Print the statistics, regardless of OPTION_STATISTICS */ 
		static void print(bool allow_unterminated_jobs= false); 
	} statistics;

	static class Signal
	{
	public:
		Signal(); 
	} signal;

public:

	Job():  pid(-2) { }

	/* Call after having returned this process from wait_do(). 
	 * Return TRUE of the child was successful. 
	 */
	bool waited(int status, pid_t pid_check) 
	{
		assert(pid_check >= 0);
		assert(pid >= 0); 
		(void) pid_check;
		assert(pid_check == pid); 
		pid= -1;
		bool success= WIFEXITED(status) && WEXITSTATUS(status) == 0;
		if (success)
			++ count_jobs_success;
		else
			++ count_jobs_fail; 
		return success; 
	}

	bool started() const {
		return pid >= 0;
	}

	bool started_or_waited() const {
		return pid >= -1; 
	}

	/* Must be started */ 
	pid_t get_pid() const {
		assert(pid >= 0);
		return pid;
	}

	/* Start the process.  Don't output the command -- this 
	 * is done by callers of this functions.   FILENAME_OUTPUT and
	 * FILENAME_INPUT are the files into which to redirect output and
	 * input; no redirection is performed when either is empty. 
	 * On error, output a message and return -1, otherwise return the
	 * PID (>= 0).  MAPPING contains the environment variables to set. 
	 */
	pid_t start(string command, 
		    const map <string, string> &mapping,
		    string filename_output,
		    string filename_input,
		    const Place &place_command); 

	/* Wait for the next process to terminate; provide the STATUS as
	 * used in wait(2).  Return the PID of the waited-for process (>=0). */  
	static pid_t wait(int *status);

	/* Block interrupt signals for the lifetime of an object of this type. 
	 * Note that the mask of blocked signals is inherited over
	 * exec(), so we must unblock signals also when starting child
	 * processes. 
	 */
	class Signal_Blocker
	{
	private:
		
#ifndef NDEBUG
		static bool blocked; 
#endif	

	public:
		Signal_Blocker() {
#ifndef NDEBUG
			blocked= true;
#endif
			if (0 != sigprocmask(SIG_BLOCK, &set_interrupt, nullptr)) {
				perror("sigprocmask");
				throw ERROR_FATAL;
			}
		}
		~Signal_Blocker() {
#ifndef NDEBUG
			blocked= false;
#endif 
			if (0 != sigprocmask(SIG_UNBLOCK, &set_interrupt, nullptr)) {
				perror("sigprocmask");
				throw ERROR_FATAL; 
			}
		}
	};
};

unsigned Job::count_jobs_exec=    0;
unsigned Job::count_jobs_success= 0;
unsigned Job::count_jobs_fail=    0;

sigset_t Job::set_block;
sigset_t Job::set_interrupt;

Job::Statistics Job::statistics;
Job::Signal     Job::signal;

sig_atomic_t Job::in_child= 0; 

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

	/* Like Make, we don't use the variable $SHELL, but use
	 * "/bin/sh" as a shell instead.  Note that the variable $SHELL
	 * is intended to denote the user's chosen interactive shell,
	 * and may not be a bourne-compatible shell.  Note also that
	 * POSIX prescribes that Make use "/bin/sh" by default. 
	 * Other note:  Make allows to declare the Make variable $SHELL
	 * within the Makefile or in Make's parameters to a value that
	 * *will* be used by Make instead of /bin/sh.  This is not possible
	 * with Stu, because Stu does not have its own
	 * set of variables.  Instead, there is the $STU_SHELL variable. 
	 */
	static const char *shell= nullptr;
	if (shell == nullptr) {
		shell= getenv("STU_SHELL");
		if (shell == nullptr || shell[0] == '\0') 
			shell= "/bin/sh"; 
	}
	
	const char *arg= command.c_str(); 
	/* c_str() never returns nullptr, as by the standard */ 
	assert(arg != nullptr);

	int fd_input= -1;
	if (filename_input != "") {
		fd_input= open(filename_input.c_str(), O_RDONLY); 
		if (fd_input < 0) {
			perror(filename_input.c_str());
			pid= -1; 
			return -1; 
		}
	}

	pid= fork();

	if (pid < 0) {
		perror("fork"); 
		assert(pid == -1); 
		return -1; 
	}

	/* Each child process is given, as process group ID, its process
	 * ID.  This ensures that we can kill each child by killing its
	 * corresponding process group ID. 
	 */

	/* Execute this in both the child and parent */ 
	int pid_child= pid;
	if (pid_child == 0)
		pid_child= getpid();
	if (0 > setpgid(pid_child, pid_child)) {
		/* This should only fail when we are the parent and the
		 * child has already quit.  In that case we can ignore
		 * the error, since the child is dead anyway, so there
		 * is no need to kill it in the future */ 
	}

	if (pid == 0) {
		/* We are the child process */ 

		/* Instead of throwing exceptions, use perror() and
		 * _Exit().  We return with ERROR_BUILD because only the
		 * child process is aborted, not Stu. 
		 */ 

		in_child= 1; 

		/* Unblock interruption signals */ 
		if (0 != sigprocmask(SIG_UNBLOCK, &set_interrupt, nullptr)) {
			perror("sigprocmask");
			_Exit(ERROR_BUILD); 
		}

		/* Set variables */ 
		size_t v_old= 0;

		/* Index of old variables */ 
		unordered_map <string, int> old;
		while (envp_global[v_old]) {
			const char *p= envp_global[v_old];
			const char *q= p;
			while (*q && *q != '=')  ++q;
			string key_old(p, q-p);
			old[key_old]= v_old;
			++v_old;
		}

		/* Maximal size of added variables.  The "+1" is for $STU_STATUS */ 
		const size_t v_new= mapping.size() + 1; 

		const char** envp= (const char **)
			alloca(sizeof(char **) * (v_old + v_new + 1));
		if (!envp) {
			/* alloca() should never return nullptr */ 
			assert(false);
			perror("alloca");
			_Exit(ERROR_BUILD); 
		}
		memcpy(envp, envp_global, v_old * sizeof(char **)); 
		size_t i= v_old;
		for (auto j= mapping.begin();  j != mapping.end();  ++j) {
			string key= j->first;
			string value= j->second;
			assert(key.find('=') == string::npos); 
			char *combined;
			if (0 > asprintf(&combined, "%s=%s", key.c_str(), value.c_str())) {
				perror("asprintf");
				_Exit(ERROR_BUILD); 
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
		 * and the column number.
		 * This makes the shell if it reports an error make the
		 * most useful output. 
		 */
		const string argv0= place_command.as_string_nocolumn(); 

		/* The one-character options to the shell */
		const char *shell_options= option_individual ? "-ex" : "-e"; 

		/* We use the -e option ('error'), which makes the shell abort
		 * on a command that fails.  This is also what POSIX prescribes
		 * for Make.  It is particularly important for Stu, as Stu
		 * invokes the whole (possibly multiline) command in one step. */
		const char *argv[]= {argv0.c_str(), 
				     shell_options, "-c", arg, nullptr, nullptr}; 

		if (arg[0] == '-' || arg[0] == '+') {
			/* Command starts with '-':  insert the parameter '--'
			 * before it so the shell does not interpret it as an
			 * option. */ 
			argv[3]= "--";
			argv[4]= arg;
		}

		/* Output redirection */
		if (filename_output != "") {
			int fd_output= creat(filename_output.c_str(), S_IRUSR | S_IWUSR); 
			if (fd_output < 0) {
				perror(filename_output.c_str());
				_Exit(ERROR_BUILD); 
			}
			int r= dup2(fd_output, 1); /* 1 = file descriptor of STDOUT */ 
			if (r < 0) {
				perror(filename_output.c_str());
				_Exit(ERROR_BUILD); 
			}
		}

		/* Input redirection */
		if (filename_input != "") {
			int r= dup2(fd_input, 0); /* 0 = file descriptor of STDIN */  
			if (r < 0) {
				perror(filename_input.c_str());
				_Exit(ERROR_BUILD); 
			}
		}

		int r= execve(shell, (char *const *) argv, (char *const *) envp); 
		/* If execve() returns, there is an error, and its return value is -1. */

		assert(r == -1); 
		perror("execve");
		_Exit(ERROR_BUILD); 
	} 

	/* Parent execution */
	++ count_jobs_exec;

	assert(pid >= 1); 
	return pid; 
}

pid_t Job::wait(int *status)
{
 begin: 	

	/* First, try wait() without blocking */ 
	pid_t pid= waitpid(-1, status, WNOHANG);
	if (pid < 0) {
		/* Should not happen as there is always something
		 * running when this function is called.  However, this may be
		 * common enough that we may want Stu to act correctly. */ 
		assert(false); 
		perror("waitpid"); 
		abort(); 
	}
	
	if (pid > 0)
		return pid; 

	/* Any SIGCHILD sent after the last call to sigwaitinfo() will
	 * be ready for receiving, even those received between the last
	 * call to waitpid() and the following call to sigwaitinfo().
	 * This excludes a deadlock which would be possible if we would
	 * only use sigwaitinfo(). */

	siginfo_t siginfo; 
	int r;
 retry:
	r= sigwaitinfo(&set_block, &siginfo);

	if (r < 0) {
		if (errno == EINTR)
			goto retry;
		else {
			assert(false);
			perror("sigwaitinfo");
			abort(); 
		}
	}

	switch (siginfo.si_signo) {

	case SIGCHLD:
		/* We could get the PID and STATUS from siginfo, but
		 * then the process would stay a zombie.  Therefore, we
		 * have to call waitpid() anyway. */ 
		goto begin; 

	case SIGUSR1:
		Statistics::print(true); 
		job_print_jobs(); 
		goto retry; 

	default:
		/* We didn't wait for that signal */ 
		assert(false);
		fprintf(stderr, "*** sigwaitinfo: Received %d\n", siginfo.si_signo);
		goto begin; 
	}
}

void Job::Statistics::print(bool allow_unterminated_jobs)
{
	/* Avoid double writing in case the destructor gets still called */ 
	option_statistics= false;
			
	struct rusage usage;

	int r= getrusage(RUSAGE_CHILDREN, &usage);
	if (r < 0) {
		perror("getrusage");
		throw ERROR_FATAL; 
	}

	if (! allow_unterminated_jobs)
		assert(count_jobs_exec == count_jobs_success + count_jobs_fail); 
	assert(count_jobs_exec >= count_jobs_success + count_jobs_fail); 

	if (! allow_unterminated_jobs) 
		printf("STATISTICS  number of jobs started = %u (%u succeeded, %u failed)\n", 
		       count_jobs_exec, count_jobs_success, count_jobs_fail); 
	else 
		printf("STATISTICS  number of jobs started = %u (%u succeeded, %u failed, %u running)\n", 
		       count_jobs_exec, count_jobs_success, count_jobs_fail, 
		       count_jobs_exec - count_jobs_success - count_jobs_fail); 

	printf("STATISTICS  children user   runtime = %ju.%06u s\n", 
	       (intmax_t) usage.ru_utime.tv_sec,
	       (unsigned) usage.ru_utime.tv_usec); 
	printf("STATISTICS  children system runtime = %ju.%06u s\n", 
	       (intmax_t) usage.ru_stime.tv_sec,
	       (unsigned) usage.ru_stime.tv_usec); 
	printf("STATISTICS  Note: children runtimes exclude running jobs\n"); 
}

/* The signal handler -- terminate all processes and quit. 
 */
void Job::handler_interrupt(int sig)
{
	/* We can use only async signal-safe functions here */

	/* Reset the signal to its default action */ 
	struct sigaction act;
	act.sa_handler= SIG_DFL;
	if (0 != sigemptyset(&act.sa_mask))  {
		write_safe(2, "*** error: sigemptyset\n"); 
	}
	act.sa_flags= SA_NODEFER;
	int r= sigaction(sig, &act, nullptr);
	assert(r == 0); 

	/* If in the child process (the short time between fork() and
	 * exec()), just quit */ 
	if (Job::in_child == 0) {
		/* Terminate all processes */ 
		job_terminate_all();
	} else {
		assert(Job::in_child == 1);
	}

	/* We cannot call Job::Statsitics::print() here because
	 * getrusage() is not async signal safe, and because the count_*
	 * variables are not atomic.  */

	/* Raise signal again */ 
	int rr= raise(sig);
	if (rr != 0) {
		write_safe(2, "*** error: raise\n"); 
	}
	
	/* Don't abort here -- the reraising of this signal may only be
	 * delivered after this handler is done. */ 
}

Job::Signal::Signal()
{
	struct sigaction act_interrupt;
	act_interrupt.sa_handler= handler_interrupt;
	if (0 != sigemptyset(&act_interrupt.sa_mask))  {
		perror("sigemptyset");
		exit(ERROR_FATAL); 
	}
	act_interrupt.sa_flags= 0; 

	if (0 != sigemptyset(&set_interrupt))  {
		perror("sigemptyset");
		exit(ERROR_FATAL); 
	}

	/* These are all signals that by default would terminate the process */ 
	/* Note:  Bash does something very similar. 
	 */
	int signals[]= { SIGTERM, SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGPIPE, SIGILL, SIGHUP };
	for (unsigned i= 0;  i < sizeof(signals) / sizeof(int);  ++i) {
		if (0 != sigaction(signals[i], &act_interrupt, nullptr)) {
			perror("sigaction");
			exit(ERROR_FATAL); 
		}
		if (0 != sigaddset(&set_interrupt, signals[i])) {
			perror("sigaddset");
			exit(ERROR_FATAL); 
		}
	}
	
	/* Block signals so we can use sigwait() to receive them */ 
	if (0 != sigemptyset(&set_block)) {
		perror("sigemptyset");
		exit(ERROR_FATAL); 
	}
	if (0 != sigaddset(&set_block, SIGCHLD)) {
		perror("sigaddset");
		exit(ERROR_FATAL);
	}
	if (0 != sigaddset(&set_block, SIGUSR1)) {
		perror("sigaddset");
		exit(ERROR_FATAL); 
	}
	if (0 != sigprocmask(SIG_BLOCK, &set_block, nullptr)) {
		perror("sigprocmask");
		exit(ERROR_FATAL); 
	}
}

#endif /* ! JOB_HH */
