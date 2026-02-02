#include "terminate_jobs.hh"

#include "file_executor.hh"

void terminate_jobs(bool asynch)
/* [ASYNC-SIGNAL-SAFE] If ASYNCH, we use only async signal-safe functions */
{
	int errno_save= errno;

	if (asynch)
		write_async(2, PACKAGE ": terminating all jobs\n");
	else
		print_error_reminder("terminating all jobs");

	/* We have two separate loops, one for killing all jobs, and one
	 * for removing all target files.  This could also be merged
	 * into a single loop. */

	for (size_t i= 0; i < File_Executor::executors_by_pid_size; ++i) {
		const pid_t pid= File_Executor::executors_by_pid_key[i];
		Job::kill(pid);
	}

	size_t count_terminated= 0;

	for (size_t i= 0; i < File_Executor::executors_by_pid_size; ++i) {
		if (File_Executor::executors_by_pid_value[i]->remove_if_existing(false))
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
