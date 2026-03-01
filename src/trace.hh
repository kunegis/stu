#ifndef TRACE_HH
#define TRACE_HH

/*
 * Tracing is a debugging tool that consist in outputting log line from certain
 * functions.  Tracing is only available in debug mode.  Tracing is used to develop Stu
 * and therefore, the choice of which functions have tracing depends on which parts have
 * needed it in the past.
 *
 * There are no trace levels, and it is not possible to output traces into an arbitrary
 * file.
 *
 * Set the variable $STU_TRACE to contain trace configuration:
 *
 * Individual settings can be separated by semicolon or newlines.  Each setting is in one
 * of the form:
 *
 *     NAME+ = LEVEL
 *     NAME+                   # LEVEL is '1'
 *     LEVEL                   # NAME is 'all'
 *
 * where NAME is the name of a source file (without src/ or .cc/.hh), and LEVEL is:
 *     0            Disabled
 *     1            Ouptut to stderr
 *     @            Output to logfile log/trace.log
 *     @<filename>  Output to given file (not implemented)
 *
 * NAME can also be 'all' to enable traces for all source files.
 *
 * EXAMPLES
 *
 *     STU_TRACE=1              Enable all traces on stderr
 *     STU_TRACE=all=1          Enable all traces on stderr
 *     STU_TRACE=@              Send all traces to logfile log/trace.log
 *     STU_TRACE=all=@          Send all traces to logfile log/trace.log
 *     STU_TRACE=dep            Send only traces for dep.cc/hh to stderr
 *     STU_TRACE=dep=1          Send only traces for dep.cc/hh to stderr
 *     STU_TRACE=dep=@          Send only traces for dep.cc/hh to logfile
 *     STU_TRACE=all;dep=0      Enable all traces except dep.cc/hh
 *     STU_TRACE='dep executor' Enable traces for dep.cc/hh and executor.cc/hh
 *     STU_TRACE='dep executor=@'
 *                              Send traces for  dep.cc/hh and executor.cc/hh to logfile
 *     STU_TRACE='all=@;dep=1'
 *                              Send all traces to logfile, except for dep.cc/hh to stderr
 */

#ifndef NDEBUG

#include <stdio.h>
#include <string.h>
#include <map>

class Trace
{
public:
	class Object
	{
	public:
		const string s;
		Object(string _s= ""): s(_s) {}
	};

	string trace_class;
	FILE *file;
	static std::map <string, FILE *> files;
	static constexpr const char *print_format= "%s:%d:%*s %s\n";

	Trace(const char *function_name, const char *filename, int line, Object);
	~Trace();
	const char *get_prefix() const { return prefix.c_str(); }

	static Trace *get_current() {
		assert(! stack.empty());
		return stack[stack.size() - 1];
	}

	static const char *strip_dir(const char *s);

	template <typename... Args>
	static void trace(const char *filename, int line, Args... args);

private:
	static constexpr int place_len= 30;
	static constexpr const char *padding_one= "|   ";
	static constexpr const char *trace_filename= "log/trace.log";
	static constexpr const char *ENV_STU_TRACE= "STU_TRACE";
	static constexpr const char *TRACE_CLASS_ALL= "all";

	string prefix;
	static string padding;
	static std::vector <Trace *> stack;
	static FILE *file_log;
	static bool global_done;

	void init_file();

	static void print(FILE *file, const char *filename, int line, const char *text);
	static FILE *open_logfile(const char *filename);
	static string normalize_trace_class(const char *trace_class);
	static void init_global();
	static void init_single(string trace_class, const char *value);
	static void error(string message= "");
};

#define TRACE_FUNCTION(a)  Trace trace_object(__func__, __FILE__, __LINE__, Trace::Object(a))

#define TRACE(...) Trace::trace(__FILE__, __LINE__, __VA_ARGS__)

#else /* NDEBUG */
#	define TRACE_FUNCTION(a)
#	define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACE_HH */
