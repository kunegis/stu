#include <locale.h>
#include <string>
#include <memory>

using std::string;
using std::shared_ptr;

#include "bits.cc"
#include "buffer.cc"
#include "buffering.cc"
#include "canonicalize.cc"
#include "color.cc"
#include "concat_executor.cc"
#include "debug.cc"
#include "dep.cc"
#include "done.cc"
#include "dynamic_executor.cc"
#include "error.cc"
#include "executor.cc"
#include "explain.cc"
#include "file_executor.cc"
#include "flags.cc"
#include "format.cc"
#include "hash_dep.cc"
#include "hints.cc"
#include "invocation.cc"
#include "job.cc"
#include "options.cc"
#include "parser.cc"
#include "preset.cc"
#include "proceed.cc"
#include "root_executor.cc"
#include "rule.cc"
#include "show.cc"
#include "signal.cc"
#include "target.cc"
#include "terminate_jobs.cc"
#include "timestamp.cc"
#include "token.cc"
#include "tokenizer.cc"
#include "trace.cc"
#include "trace_dep.cc"
#include "trace_executor.cc"
#include "transient_executor.cc"

int main(int argc, char **argv, char **envp)
{
	TRACE_FUNCTION();
	dollar_zero= argv[0];
	envp_global= (const char **) envp;
	setlocale(LC_CTYPE, ""); /* Tokenizer::current_mbchar() */
	init_buffering();
	Color::set();
	set_env_options();
	check_status();
	int error= 0;

	try {
		Invocation invocation(argc, argv, error);
		invocation.main_loop();
	} catch (int e) {
		assert(e >= 1 && e <= 3);
		error= e;
	}

	if (option_z)
		Job::print_statistics();
	if (fclose(stdout)) {
		print_errno("fclose", "<stdout>");
		exit(ERROR_FATAL);
	}
	/* No need to flush stderr, because it is line buffered, and if we used it, it
	 * means there was an error anyway, so we're not losing any information. */
	exit(error);
}
