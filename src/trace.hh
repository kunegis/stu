#ifndef TRACE_HH
#define TRACE_HH

/*
 * Tracing is a debugging tool that consist in outputting log line from certain
 * functions.  Tracing is only available in debug mode.  Tracing is used to develop Stu
 * and therefore, the choice of which functions have tracing depends on which parts have
 * needed it in the past.
 *
 * To enable tracing for a trace class TRACE_ABC, set the environment variable
 * $STU_TRACE_ABC to:
 *	"log"     Write into the trace logfile (see name below)
 *	"stderr"  Write on stderr
 *	"off"     No tracing (same as variable not set)
 *
 * There are no trace levels, and it is not possible to output traces in an arbitrary
 * file.
 */

#ifndef NDEBUG

#include <string.h>

enum Trace_Class {
	TRACE_EXECUTOR, TRACE_SHOW, TRACE_TOKENIZER, TRACE_DEP,
	TRACE_COUNT
};

class Trace
{
public:
	Trace_Class trace_class;
	static FILE *trace_files[TRACE_COUNT];

	Trace(Trace_Class trace_class_, const char *name, const char *file, int line);
	~Trace();
	const char *get_prefix() const { return prefix.c_str(); }
	bool get_enabled() const { return trace_files[trace_class]; }

	static constexpr const char *print_format= "%21s:%-4d %s\n";

	static Trace *get_current() {
		assert(! stack.empty());
		return stack[stack.size() - 1];
	}

	static const char *strip_dir(const char *s);

private:
	string prefix;
	static string padding;
	static std::vector <Trace *> stack;

	static constexpr const char *padding_one= "   ";
	static constexpr const char *trace_filename= "log/trace.log";

	static struct Init { Init(); } init;

	static FILE *open_logfile(const char *filename);
};

#define TRACE_FUNCTION(CLASS, NAME)  Trace trace_object(TRACE_ ## CLASS, #NAME, __FILE__, __LINE__)

#define TRACE(format, ...)  { \
	if (Trace::get_current()->get_enabled()) { \
		string trace_text= fmt("%s" format, \
				       trace_object.get_prefix(), \
				       __VA_ARGS__); \
		if (fprintf(Trace::trace_files[Trace::get_current()->trace_class], \
			    Trace::print_format, \
			    Trace::strip_dir(__FILE__), __LINE__, \
			    trace_text.c_str()) == EOF) { \
			perror("fprintf(trace_file)"); \
			exit(ERROR_FATAL); \
		} \
	} \
}

#else /* NDEBUG */
#	define TRACE_FUNCTION(a, b)
#	define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACE_HH */
