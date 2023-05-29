#ifndef TRACING_HH
#define TRACING_HH

// TODO rename to "trace".

/*
 * To enable tracing for a trace class TRACE_ABC, set the environment variable
 * $STU_TRACE_ABC to:
 * 	"log" 	  Write into trace logfile
 *	"stderr"  Write on stderr  
 * 	"off"	  No tracing (same as not variable set)
 */

#ifndef NDEBUG

#include <string.h>

#define TRACING_FILE "log/trace.log"

enum Tracing_Class
{
	TRACING_SHOW,
	TRACING_COUNT
};

const char *trace_names[TRACING_COUNT]= {"SHOW"};

class Tracing
{
public:
	Tracing_Class tracing_class;
	static FILE *trace_files[TRACING_COUNT];

	Tracing(Tracing_Class tracing_class_, const char *name)
		:  tracing_class(tracing_class_)
	{
		assert(tracing_class >= 0 && tracing_class < TRACING_COUNT); 
		stack.push_back(this);
		if (! (trace_files[tracing_class]))
			return;
		prefix= padding + name + ' ';
		padding += padding_one;
	}

	~Tracing() {
		stack.pop_back();
		if (! (trace_files[tracing_class]))
			return;
		padding.resize(padding.size() - strlen(padding_one));
	}

	const char *get_prefix() const {
		return prefix.c_str();
	}

	bool get_enabled() const {
		return trace_files[tracing_class];
	}

	static Tracing *get_current() {
		assert(! stack.empty());
		return stack[stack.size() - 1];
	}
	
private:
	string prefix;
	static string padding;
	static constexpr const char *padding_one= "   ";
//	static bool enabled[TRACING_COUNT];
	static vector <Tracing *> stack;

	static struct Init  {  Init();  }  init;

	static FILE *open_logfile(const char *filename);
};

// TODO the two following macros don't have to be macros.  Make them be
// functions. 

// TODO the first argument should be the constant.
// TODO the second argument should be the string.
#define TRACE_FUNCTION(CLASS, NAME)  Tracing tracing_object(TRACING_ ## CLASS, #NAME)

#define TRACE(format, ...)  { \
	if (Tracing::get_current()->get_enabled()) {	\
		string trace_text= fmt("%s" format "\n", \
			tracing_object.get_prefix(), \
			__VA_ARGS__); \
		if (fputs(trace_text.c_str(), Tracing::trace_files[Tracing::get_current()->tracing_class]) == EOF) { \
			perror("fputs(..., trace_file)");		\
			exit(ERROR_FATAL); \
		} \
	} \
}

#else /* NDEBUG */
#define TRACE_FUNCTION(a, b)
#define TRACE(a, ...)
#endif /* NDEBUG */

#endif /* ! TRACING_HH */
