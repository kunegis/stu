#ifndef MAIN_LOOP_HH
#define MAIN_LOOP_HH

/*
 * Main execution loop.  This throws ERROR_BUILD and ERROR_LOGICAL.
 */

void main_loop(const vector <shared_ptr <const Dep> > &deps)
{
	assert(options_jobs >= 0);
	Root_Execution *root_execution= new Root_Execution(deps);
	int error= 0;
	shared_ptr <const Root_Dep> dep_root= make_shared <Root_Dep> ();

	try {
		while (! root_execution->finished()) {
			Proceed proceed;
			do {
				Debug::print(nullptr, "loop");
				proceed= root_execution->execute(dep_root);
				assert(proceed);
			} while (proceed & P_PENDING);

			if (proceed & P_WAIT) {
				File_Execution::wait();
			}
		}

		assert(root_execution->finished());
		assert(File_Execution::executions_by_pid_size == 0);

		bool success= (root_execution->get_error() == 0);
		assert(option_keep_going || success);

		error= root_execution->get_error();
		assert(error >= 0 && error <= 3);

		if (success) {
			if (! Execution::hide_out_message) {
				if (Execution::out_message_done)
					print_out("Build successful");
				else
					print_out("Targets are up to date");
			}
		} else {
			if (option_keep_going) {
				print_error_reminder("Targets not up to date because of errors");
			}
		}
	}

	/* A build error is only thrown when option_keep_going is
	 * not set */
	catch (int e) {
		assert(! option_keep_going);
		assert(e >= 1 && e <= 4);
		/* Terminate all jobs */
		if (File_Execution::executions_by_pid_size) {
			print_error_reminder("Terminating all jobs");
			job_terminate_all();
		}
		assert(e > 0 && e < ERROR_FATAL);
		error= e;
	}

	if (error)
		throw error;
}

#endif /* ! MAIN_LOOP_HH */
