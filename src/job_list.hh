#ifndef JOB_LIST_HH
#define JOB_LIST_HH

#include "file_executor.hh"

class Job_List
{
public:
	static File_Executor *find(pid_t pid, size_t &index);
	static size_t get_size() { return size; }
	static void add(pid_t pid, size_t &index, File_Executor *executor);
	static void remove(size_t index);
	static void print();
	static void terminate_jobs(bool asynch);

#ifndef NDEBUG
	static File_Executor *get(size_t index);
#endif /* ! NDEBUG */

private:
	static size_t size;
	static pid_t *pids;
	static File_Executor **executors;
	/* The currently running executors by process IDs.  Write access to this is
	 * enclosed in a Signal_Blocker.  Both arrays are malloc'ed, have the same length,
	 * and are both sorted by PID.  malloc() is only called once for each array,
	 * giving the allocated memory a length that will be enough for all jobs we will
	 * ever run, based on the value passed via the -j option, so we avoid excessive
	 * calling of realloc(), and race conditions while accessing this.  For all file
	 * executors stored here, the following variables are never changed as long as the
	 * File_Executor objects are stored there, such that they can be accessed from
	 * async-signal safe functions:  FILENAMES, TIMESTAMPS_OLD. */
};

#endif /* ! JOB_LIST_HH */
