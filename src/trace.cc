#include "trace.hh"

#ifndef NDEBUG

FILE *Trace::trace_files[TRACE_COUNT];
string Trace::padding;
Trace::Init Trace::init;
vector <Trace *> Trace::stack;

Trace::Init::Init()
{
	FILE *file_log= nullptr;
	FILE *file_devnull= nullptr;

	for (int i= 0; i < TRACE_COUNT; ++i) {
		string name= fmt("STU_TRACE_%s", trace_names[i]);
		const char *env= getenv(name.c_str());
		bool enabled= (env && env[0]);
		if (!enabled) {
			if (!file_devnull)
				file_devnull= open_logfile("/dev/null");
			trace_files[i]= file_devnull;
		} else if (!strcmp(env, "log")) {
			if (!file_log)
				file_log= open_logfile(TRACE_FILE);
			trace_files[i]= file_log;
		} else if (!strcmp(env, "stderr")) {
			trace_files[i]= stderr;
		} else if (!strcmp(env, "off")) {
			trace_files[i]= nullptr;
		} else {
			fprintf(stderr, "*** Error: Invalid value for trace %s=%s\n",
				name.c_str(), env);
			exit(ERROR_FATAL);
		}
	}
}

FILE *Trace::open_logfile(const char *filename)
{
	FILE *ret= fopen(filename, "w");
	if (!ret) {
		print_errno(fmt("fopen(%s)", filename));
		exit(ERROR_FATAL);
	}
	int flags= fcntl(fileno(ret), F_GETFL, 0);
	if (flags >= 0)
		fcntl(fileno(ret), F_SETFL, flags | O_APPEND | FD_CLOEXEC);
	assert(ret);
	return ret;
}

const char *trace_strip_dir(const char *s)
{
	const char *r= strchr(s, '/');
	if (!r)  return s;
	return r+1;
}

#endif /* ! NDEBUG */
