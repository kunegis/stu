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

class Trace
{
public:
	string trace_class;
	FILE *file;
	static std::map <string, FILE *> files;
	static constexpr const char *print_format= "%21s:%-4d %s\n";

	Trace(const char *function_name, const char *filename, int line);
	~Trace();
	const char *get_prefix() const { return prefix.c_str(); }

	static Trace *get_current() {
		assert(! stack.empty());
		return stack[stack.size() - 1];
	}

	static const char *strip_dir(const char *s);

private:
	string prefix;
	static string padding;
	static std::vector <Trace *> stack;
	static FILE *file_log;
	static constexpr const char *padding_one= "   ";
	static constexpr const char *trace_filename= "log/trace.log";

	void init_file();
	static FILE *open_logfile(const char *filename);
	static string class_from_filename(const char *filename);
};

#define TRACE_FUNCTION()  Trace trace_object(__func__, __FILE__, __LINE__)

#define TRACE(format, ...)  {						\
		if (FILE *file= Trace::get_current()->file) {		\
		string trace_text= fmt("%s" format,			\
			trace_object.get_prefix(),			\
			__VA_ARGS__);					\
		if (fprintf(file,					\
				Trace::print_format,			\
				Trace::strip_dir(__FILE__), __LINE__,	\
				trace_text.c_str()) == EOF) {		\
			perror("fprintf(trace_file)");			\
			exit(ERROR_FATAL);				\
		} \
	} \
}

#else /* NDEBUG */
#	define TRACE_FUNCTION(a)
#	define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACE_HH */
