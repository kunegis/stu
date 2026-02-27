#include "trace.hh"

#ifndef NDEBUG

#include <algorithm>

std::map <string, FILE *> Trace::files;
string Trace::padding;
std::vector <Trace *> Trace::stack;
FILE *Trace::file_log= nullptr;
bool Trace::global_done= false;

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
	init_global();
	auto i= files.find(trace_class);
	if (i != files.end()) {
		file= i->second;
		return;
	}

	string name= fmt("STU_TRACE_%s", trace_class);
	string name_all= ENV_STU_TRACE_ALL;
	string vars[]= {name, name_all};
	files[trace_class]= file= nullptr;
	for (const string &var: vars) {
		const char *env= getenv(var.c_str());
		if (init_single(trace_class, env)) break;
	}
	i= files.find(trace_class);
	if (i != files.end()) {
		file= i->second;
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

void Trace::init_global()
{
	if (global_done) return;
	const char *env= getenv(ENV_STU_TRACE);
	if (!env) return;
	for (;;) {
		while (isspace(*env) || *env == ';') ++env;
		if (!*env) break;
		std::vector <string> trace_classes;
		while (*env && *env != ';') {
			const char *p= env;
			while (*p >= 'A' && *p <= 'Z' || *p == '_') ++p;
			if (p == env) {
				print_error(fmt("invalid value in $%s (1)", ENV_STU_TRACE));
				error_exit();
			}
			trace_classes.push_back(string(env, p-env));
			env= p;
			while (isspace(*env)) ++env;
		}
		while (isspace(*env)) ++env;
		string value;
		if (*env == ';' || !*env) {
			value= "stderr";
		} else if (*env == '=') {
			++env;
			while (isspace(*env)) ++env;
			const char *p= env;
			while (isalnum(*p)) ++p;
			if (p == env) {
				print_error(fmt("invalid value in $%s (3)", ENV_STU_TRACE));
				error_exit();
			}
			value= string(env, p);
			env= p;
			while (isspace(*env)) ++env;
		} else {
			print_error(fmt("invalid value in $%s (2)", ENV_STU_TRACE));
			error_exit();
		}
		for (string trace_class: trace_classes) {
			init_single(trace_class, value.c_str());
		}
	}
}

bool Trace::init_single(string trace_class, const char *value)
{
	if (value && (!strcmp(value, "off") || !strcmp(value, "0"))) {
		return true;
	} else if (!value || !value[0]) {
		return false;
	} else if (!strcmp(value, "log")) {
		if (!file_log)
			file_log= open_logfile(trace_filename);
		files[trace_class]= file_log;
		return false;
	} else if (!strcmp(value, "stderr") ||
		(value[0] >= '1' && value[0] <= '9' && !value[1])) {
		files[trace_class]= stderr;
		return false;
	} else {
		print_error(fmt("invalid value for trace %s: %s",
				trace_class, value));
		error_exit();
	}
}

#endif /* ! NDEBUG */
