#include "job_list.hh"

size_t Job_List::size= 0;
pid_t *Job_List::pids= nullptr;
File_Executor **Job_List::executors= nullptr;

File_Executor *Job_List::find(pid_t pid, size_t &index)
{
	assert(size);
	assert(pids);
	assert(executors);

	size_t mi= 0, ma= size - 1;
	/* Both are inclusive */

	assert(mi <= ma);
	while (mi < ma) {
		size_t ne= mi + (ma - mi + 1) / 2;
		assert(ne <= ma);
		if (pids[ne] == pid) {
			mi= ma= ne;
			break;
		}
		if (pids[ne] < pid) {
			mi= ne + 1;
		} else {
			ma= ne - 1;
		}
	}
	if (mi > ma || mi == SIZE_MAX) {
		/* No File_Executor is registered for the PID that just finished.  Should
		 * not happen, but since the PID value came from outside this process, we
		 * better handle this case gracefully, i.e., do nothing. */
		should_not_happen();
		print_warning(Place(),
			frmt("the function waitpid(2) returned the invalid process ID %jd",
				(intmax_t)pid));
		return nullptr;
	}

	assert(mi == ma);
	index= mi;
	assert(index < size);
	assert(pids[index] == pid);
	assert(executors[index]);
	return executors[index];
}

void Job_List::add(pid_t pid, size_t &index, File_Executor *executor)
{
	TRACE_FUNCTION();
	TRACE("pid= %s", frmt("%jd", (intmax_t)pid));
	assert(Signal_Blocker::is_blocked());
	assert(!pids == !executors);

	if (!pids) {
		/* This is executed just once, before we have executed any job, and
		 * therefore JOBS is the value passed via -j (or its default value 1), and
		 * thus we can allocate arrays of that size once and for all. */
		if ((uintmax_t)SIZE_MAX / sizeof(*pids) < (uintmax_t)options_jobs ||
			(uintmax_t)SIZE_MAX / sizeof(*executors) < (uintmax_t)options_jobs)
		{
			happens_only_on_certain_platforms();
			/* This can only happen when long is at least as large as size_t,
			 * which is not the case on any commonly used platform, but is
			 * allowed by ISO C++ and POSIX. */
			errno= ENOMEM;
			print_errno_bare(frmt(
				"Value too large for option -j, maximum value is %ju",
				(uintmax_t)SIZE_MAX / std::max(sizeof(*pids),
				sizeof(*executors))));
			error_exit();
		}
		cov_tag("Job_List::add");
		pids= (pid_t *)malloc(options_jobs * sizeof(*pids));
		executors= (File_Executor **)
			malloc(options_jobs * sizeof(*executors));
		if (!pids || !executors) {
			print_errno("malloc");
			error_exit();
		}
	}

	size_t index_new= size++;
	while (index_new && pids[index_new - 1] > pid) {
		pids[index_new]= pids[index_new - 1];
		executors[index_new]= executors[index_new - 1];
		--index_new;
	}
	pids[index_new]= pid;
	executors[index_new]= executor;
	index= index_new;
}

void Job_List::remove(size_t index)
{
	TRACE_FUNCTION();
	TRACE("index= %s", frmt("%zu", index));
	assert(Signal_Blocker::is_blocked());
	assert(size);
	assert(size >= index + 1);
	memmove(pids + index,
		pids + index + 1,
		sizeof(*pids) * (size - index - 1));
	memmove(executors + index,
		executors + index + 1,
		sizeof(*executors) * (size - index - 1));
	--size;
}

void Job_List::print()
{
	for (size_t i= 0; i < size; ++i)
		executors[i]->print_as_job();
}

void Job_List::terminate_jobs(bool asynch)
/* [ASYNC-SIGNAL-SAFE] If ASYNCH, we use only async signal-safe functions */
{
	int errno_save= errno;

	if (size > 1) {
		if (asynch)
			write_async(2, PACKAGE ": terminating all jobs\n");
		else
			print_error_reminder("terminating all jobs");
	}

	/* We have two separate loops, one for killing all jobs, and one for removing all
	 * target files.  This could also be merged into a single loop. */

	for (size_t i= 0; i < size; ++i) {
		const pid_t pid= pids[i];
		Job::kill(pid);
	}

	size_t count_terminated= 0;

	for (size_t i= 0; i < size; ++i) {
		if (executors[i]->remove_if_existing(false))
			++count_terminated;
	}

	if (count_terminated) {
		if (asynch) {
			write_async(2, PACKAGE ": removing partially built files (");
			/* Maximum characters in decimal representation of SIZE_T */
			constexpr size_t len= sizeof(size_t) * CHAR_BIT / 3 + 3;
			char out[len];
			out[len - 1]= '\n';
			out[len - 2]= ')';
			ssize_t i= len - 3;
			size_t n= count_terminated;
			do {
				out[i]= '0' + n % 10;
				n /= 10;
			} while (n > 0 && --i >= 0);
			ssize_t r= write(2, out + i, len - i);
			(void) r; /* There's not much we can do if write() fails */
		} else {
			print_error_reminder(
				frmt("removing partially built files (%zu)",
					count_terminated));
		}
	}

	/* Check that all children are terminated */
	while (true) {
		int status;
		int ret= wait(&status);

		if (ret < 0) {
			/* wait() sets errno to ECHILD when there was no
			 * child to wait for */
			if (errno == EINTR)
				continue;
			if (errno != ECHILD) {
				if (asynch)
					write_async(2, "stu: error: wait\n");
				else
					print_errno("wait");
			}
			break;
		}
		assert_async(ret > 0);
	}

	errno= errno_save;
}

#ifndef NDEBUG
File_Executor *Job_List::get(size_t index)
{
	assert(index < size);
	return executors[index];
}
#endif /* ! NDEBUG */
