#ifndef TRACE_HH
#define TRACE_HH

/*
 * To enable tracing for a trace class TRACE_ABC, set the environment variable
 * $STU_TRACE_ABC to:
 * 	"log" 	  Write into trace logfile
 *	"stderr"  Write on stderr
 * 	"off"	  No tracing (same as not variable set)
 */

#ifndef NDEBUG

#include <string.h>

#define TRACE_FILE "log/trace.log"

enum Trace_Class
{
	TRACE_SHOW, TRACE_TOKENIZER,

	TRACE_COUNT
};

const char *trace_names[TRACE_COUNT]= {"SHOW", "TOKENIZER"};

class Trace
{
public:
	Trace_Class trace_class;
	static FILE *trace_files[TRACE_COUNT];

	Trace(Trace_Class trace_class_, const char *name)
		:  trace_class(trace_class_)
	{
		assert(trace_class >= 0 && trace_class < TRACE_COUNT);
		stack.push_back(this);
		if (! (trace_files[trace_class]))
			return;
		prefix= padding + name + ' ';
		padding += padding_one;
	}

	~Trace() {
		stack.pop_back();
		if (! (trace_files[trace_class]))
			return;
		padding.resize(padding.size() - strlen(padding_one));
	}

	const char *get_prefix() const { return prefix.c_str(); }
	bool get_enabled() const { return trace_files[trace_class]; }

	static Trace *get_current() {
		assert(! stack.empty());
		return stack[stack.size() - 1];
	}

private:
	string prefix;
	static string padding;
	static constexpr const char *padding_one= "   ";
	static vector <Trace *> stack;

	static struct Init { Init(); } init;

	static FILE *open_logfile(const char *filename);
};

const char *trace_strip_dir(const char *s);

#define TRACE_FUNCTION(CLASS, NAME)  Trace trace_object(TRACE_ ## CLASS, #NAME)

#define TRACE(format, ...)  { \
	if (Trace::get_current()->get_enabled()) {	\
		string trace_text= fmt("%s" format, \
			trace_object.get_prefix(), \
			__VA_ARGS__); \
		if (fprintf(Trace::trace_files[Trace::get_current()->trace_class], \
			    "%21s:%-4d  %s\n", \
			    trace_strip_dir(__FILE__), __LINE__,  \
			    trace_text.c_str()) == EOF) { \
			perror("fprintf(trace_file)"); \
			exit(ERROR_FATAL); \
		} \
	} \
}

#else /* NDEBUG */
#define TRACE_FUNCTION(a, b)
#define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACE_HH */
