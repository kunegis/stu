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
	print(file, filename, line, text.c_str());
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
	string name_all= "STU_TRACE_ALL";
	string vars[]= {name, name_all};
	files[trace_class]= file= nullptr;
	for (const string &var: vars) {
		const char *env= getenv(var.c_str());
		if (env && (!strcmp(env, "off") || !strcmp(env, "0"))) {
			break;
		} else if (!env || !env[0]) {
			continue;
		} else if (!strcmp(env, "log")) {
			if (!file_log)
				file_log= open_logfile(trace_filename);
			files[trace_class]= file= file_log;
		} else if (!strcmp(env, "stderr") ||
			(env[0] >= '1' && env[0] <= '9' && !env[1])) {
			files[trace_class]= file= stderr;
		} else {
			print_error(fmt("invalid value for trace %s=%s",
					var.c_str(), env));
			error_exit();
		}
	}
}

FILE *Trace::open_logfile(const char *filename)
{
	FILE *ret= fopen(filename, "w");
	if (!ret) {
		print_errno("fopen", filename);
		error_exit();
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
	print(file, filename, line, full_line.c_str());
}

void Trace::print(FILE *file, const char *filename, int line, const char *text)
{
	assert(line > 0);
	int digits= 0;
	for (int n= line; n; n /= 10) ++digits;
	int spaces= place_len - strlen(filename) - digits;
	if (fprintf(file, Trace::print_format,
			filename, line,
			spaces, "",
			text) < 0) {
		print_errno("fprintf", filename);
		error_exit();
	}
}

#endif /* ! NDEBUG */
