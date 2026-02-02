#ifndef TRACE_HH
#define TRACE_HH

/*
 * Tracing is a debugging tool that consist in outputting log line from certain
 * functions.  Tracing is only available in debug mode.  Tracing is used to develop Stu
 * and therefore, the choice of which functions have tracing depends on which parts have
 * needed it in the past.
 *
 * To enable tracing for the file src/<name>.cc, set the environment variable
 * $STU_TRACE_<NAME> (all uppercase) to the following: (Example: use $STU_TRACE_DEP to
 * enable tracing in src/dep.{cc,hh})
 *	"1".."9"  Synonym of "stderr"
 *	"log"     Write into the trace logfile (see name below)
 *	"stderr"  Write on stderr
 *	"off"/"0" No tracing (useful to disable a single file when $STU_TRACE_ALL is set)
 *
 * Set $STU_TRACE_ALL to enable tracing in all source code files.
 *
 * There are no trace levels, and it is not possible to output traces into an arbitrary
 * file.
 */

#ifndef NDEBUG

#include <string.h>

class Trace
{
public:
	class Object
	{
	public:
		const string s;
		Object(string _s= ""): s(_s) { }
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
	string prefix;
	static string padding;
	static std::vector <Trace *> stack;
	static FILE *file_log;
	static constexpr int place_len= 30;
	static constexpr const char *padding_one= "|   ";
	static constexpr const char *trace_filename= "log/trace.log";

	void init_file();
	static void print(FILE *file, const char *filename, int line, const char *text);
	static FILE *open_logfile(const char *filename);
	static string class_from_filename(const char *filename);
};

#define TRACE_FUNCTION(a)  Trace trace_object(__func__, __FILE__, __LINE__, Trace::Object(a))

#define TRACE(...) Trace::trace(__FILE__, __LINE__, __VA_ARGS__)

#else /* NDEBUG */
#	define TRACE_FUNCTION(a)
#	define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACE_HH */
