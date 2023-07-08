#include "main_loop.hh"

void main_loop(const vector <shared_ptr <const Dep> > &deps)
{
	assert(options_jobs >= 0);
	Root_Executor *root_executor= new Root_Executor(deps);
	int error= 0;
	shared_ptr <const Root_Dep> dep_root= make_shared <Root_Dep> ();

	try {
		while (! root_executor->finished()) {
			Proceed proceed;
			do {
				Debug::print(nullptr, "loop");
				proceed= root_executor->execute(dep_root);
				assert(proceed);
			} while (proceed & P_PENDING);

			if (proceed & P_WAIT) {
				File_Executor::wait();
			}
		}

		assert(root_executor->finished());
		assert(File_Executor::executors_by_pid_size == 0);

		bool success= (root_executor->get_error() == 0);
		assert(option_k || success);

		error= root_executor->get_error();
		assert(error >= 0 && error <= 3);

		if (success) {
			if (! Executor::hide_out_message) {
				if (Executor::out_message_done)
					print_out("Build successful");
				else
					print_out("Targets are up to date");
			}
		} else {
			if (option_k) {
				print_error_reminder("targets not up to date because of errors");
			}
		}
	}

	/* A build error is only thrown when option_keep_going is not set */
	catch (int e) {
		assert(! option_k);
		assert(e >= 1 && e <= 4);
		/* Terminate all jobs */
		if (File_Executor::executors_by_pid_size) {
			print_error_reminder("terminating all jobs");
			terminate_jobs();
		}
		assert(e > 0 && e < ERROR_FATAL);
		error= e;
	}

	if (error)
		throw error;
}
