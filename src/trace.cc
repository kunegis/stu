#include "trace.hh"

#ifndef NDEBUG

#include <algorithm>

std::map <string, FILE *> Trace::files;
string Trace::padding;
std::vector <Trace *> Trace::stack;
FILE *Trace::file_log= nullptr;

Trace::Trace(const char *function_name, const char *filename, int line, Object object)
{
	const char *filename_stripped= strip_dir(filename);
	trace_class= class_from_filename(filename_stripped);
	stack.push_back(this);
	init_file();
	if (!file) return;
	string text= fmt("%s%s%s%s()",
		padding,
		object.s.empty() ? "" : object.s.c_str(),
		object.s.empty() ? "" : ".",
		function_name);
	if (fprintf(file, print_format,
			filename_stripped, line,
			text.c_str()) == EOF) {
		perror("Trace: fprintf");
		exit(ERROR_FATAL);
	}
	padding += padding_one;
	prefix= padding;
}

Trace::~Trace()
{
	stack.pop_back();
	if (!file)
		return;
	padding.resize(padding.size() - strlen(padding_one));
}

void Trace::init_file()
{
	auto i= files.find(trace_class);
	if (i != files.end()) {
		file= i->second;
		return;
	}

	string name= fmt("STU_TRACE_%s", trace_class);
	const char *env= getenv(name.c_str());
	if (!env || !env[0] || !strcmp(env, "off")) {
		files[trace_class]= file= nullptr;
	} else if (!strcmp(env, "log")) {
		if (!file_log)
			file_log= open_logfile(trace_filename);
		files[trace_class]= file= file_log;
	} else if (!strcmp(env, "stderr")) {
		files[trace_class]= file= stderr;
	} else {
		fprintf(stderr, "stu: error: invalid value for trace %s=%s\n",
			name.c_str(), env);
		exit(ERROR_FATAL);
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

const char *Trace::strip_dir(const char *s)
{
	const char *r= strchr(s, '/');
	if (!r) return s;
	return r+1;
}

string Trace::class_from_filename(const char *filename)
{
	const char *r= strchr(filename, '.');
	assert(r);
	string ret= string(filename, r - filename);
	std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
	return ret;
}

template <typename... Args>
void Trace::trace(const char *filename, int line, Args... args)
{
	FILE *file= get_current()->file;
	if (!file) return;
	string prefix= get_current()->get_prefix();
	string text= fmt(args...);
	string full_line= prefix + text;
	if (fprintf(file, Trace::print_format,
			Trace::strip_dir(filename), line,
			full_line.c_str()) == EOF) {
		perror("fprintf(trace_file)");
		exit(ERROR_FATAL);
	}
}

#endif /* ! NDEBUG */
