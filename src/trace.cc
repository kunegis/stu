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
	trace_class= normalize_trace_class(filename);
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
	file= nullptr;
	auto i= files.find(trace_class);
	if (i != files.end()) {
		file= i->second;
		return;
	}
	i= files.find(TRACE_CLASS_ALL);
	if (i != files.end()) {
		file= i->second;
		return;
	}
}

FILE *Trace::open_logfile(const char *filename)
{
	FILE *ret= fopen(filename, "w");
	if (!ret) {
		print_errno("fopen", filename);
		error();
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

string Trace::normalize_trace_class(const char *s)
{
	s= strip_dir(s);
	string ret;
	const char *r= strchr(s, '.');
	if (r)
		ret= string(s, r - s);
	else
		ret= string(s);
	std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
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
		error();
	}
}

void Trace::init_global()
{
//	fprintf(stderr, "A\n");//
	if (global_done) return;
	global_done= true;
//	fprintf(stderr, "B\n");//
	const char *env= getenv(ENV_STU_TRACE);
	if (!env) return;
	fprintf(stderr, "C env=%s\n", env);//
	for (;;) {
		fprintf(stderr, "D env=%s\n", env);//
		while (isspace(*env) || *env == ';') ++env;
		fprintf(stderr, "E env=%s\n", env);//
		if (!*env) break;
		std::vector <string> trace_classes;
		while (*env && *env != ';' && *env != '\n' && *env != '=') {
			fprintf(stderr, "F env=%s\n", env);//
			const char *p= env;
			while (*p >= 'a' && *p <= 'z' || *p == '_') ++p;
			fprintf(stderr, "Fa p=%s\n", p);//
			if (p == env) {
				fprintf(stderr, "Fb env=%s\n", p);//
				trace_classes.push_back(TRACE_CLASS_ALL);
				break;
			} else {
				fprintf(stderr, "Fc env=%s\n", p);//
				trace_classes.push_back(string(env, p-env));
				env= p;
				while (isspace(*env) && *env != '\n') ++env;
				fprintf(stderr, "G env=%s\n", env);//
			}
		}
		fprintf(stderr, "H env=%s\n", env);//
		while (isspace(*env) && *env != '\n') ++env;
		fprintf(stderr, "I env=%s\n", env);//
		string value;
		if (*env == ';' || *env == '\n' || !*env) {
			fprintf(stderr, "J env=%s\n", env);//
			value= "1";
		} else if (*env) {
			fprintf(stderr, "K env=%s\n", env);//
			if (trace_classes.size() && *env == '=') ++env;
			fprintf(stderr, "Ka env=%s\n", env);//
			while (isspace(*env) && *env != '\n') ++env;
			fprintf(stderr, "L env=%s\n", env);//
			const char *p= env;
			while (isalnum(*p)) ++p;
			fprintf(stderr, "M env=%s\n", env);//
			if (p == env) {
				error(fmt("invalid value in $%s (3)", ENV_STU_TRACE));
			}
			value= string(env, p);
			env= p;
			while (isspace(*env) || *env == '\n') ++env;
			fprintf(stderr, "N env=%s\n", env);//
		} else {
			error(fmt("invalid value in $%s (2)", ENV_STU_TRACE));
		}
		for (string trace_class: trace_classes)
			init_single(
				normalize_trace_class(trace_class.c_str()), value.c_str());
	}
}

void Trace::init_single(string trace_class, const char *value)
{
	fprintf(stderr, "init_single trace_class='%s' value='%s'\n",
		trace_class.c_str(), value);//
	if (!value || !value[0]) return;
	if (!strcmp(value, "0")) {
		files[trace_class]= nullptr;
	} else if (!strcmp(value, "@")) {
		if (!file_log)
			file_log= open_logfile(trace_filename);
		files[trace_class]= file_log;
	} else if (!strcmp(value, "1")) {
		files[trace_class]= stderr;
	} else {
		error(fmt("invalid value for trace %s: %s",
				trace_class, value));
	}
}

void Trace::error(string message)
{
	if (!message.empty()) {
		fprintf(stderr, "%s: Trace error: %s\n", dollar_zero, message.c_str());
	}
	exit(ERROR_TRACE);
}

#endif /* ! NDEBUG */
