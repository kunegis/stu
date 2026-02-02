#include "error.hh"

#include <string.h>

#include "color.hh"
#include "format.hh"
#include "options.hh"
#include "show.hh"
#include "terminate_jobs.hh"

void print_error(string message)
{
	assert(! message.empty());
	assert(islower(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: %s\n",
		Color::stderr_err_on, dollar_zero, Color::stderr_err_off, message.c_str());
}

void print_error_reminder(string message)
{
	assert(! message.empty());
	assert(islower(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: %s\n",
		Color::stderr_warn_on, dollar_zero, Color::stderr_warn_off,
		message.c_str());
}

void print_errno(const char *call)
{
	TRACE_FUNCTION();
	TRACE("call= '%s'", call);
	assert(call && call[0] && call[0] != '\033');
	fprintf(stderr, "%s%s%s: %s\n",
		Color::stderr_err_on, call, Color::stderr_err_off,
		strerror(errno));
}

void print_errno(const char *call, string filename)
{
	TRACE_FUNCTION();
	TRACE("call= '%s'", call);
	TRACE("filename= '%s'", filename);
	assert(call && call[0] && call[0] != '\033');
	assert(filename.size() > 0 && filename[0] != '\033');
	string show_filename= ::show(filename, S_QUOTE_MINIMUM);
	fprintf(stderr, "%s%s%s: %s: %s\n",
		Color::stderr_err_on, show_filename.c_str(), Color::stderr_err_off,
		call, strerror(errno));
}

void print_errno_bare(string text) /* uncovered */
{
	happens_only_on_certain_platforms();
	TRACE_FUNCTION();
	TRACE("text= '%s'", text);
	assert(text.size() > 0);
	fprintf(stderr, "%s%s%s: %s: %s\n",
		Color::stderr_err_on, dollar_zero, Color::stderr_err_off,
		text.c_str(), strerror(errno));
}

string format_errno(const char *call)
{
	return fmt("%s: %s", call, strerror(errno));
}
string format_errno(const char *call, string filename)
{
	string show_filename= ::show(filename, S_QUOTE_MINIMUM);
	return fmt("%s: %s: %s", show_filename, call, strerror(errno));
}

string format_errno_bare(string text)
{
	return fmt("%s: %s", text, strerror(errno));
}

void print_out(string text)
{
	assert(! text.empty());
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');
	if (option_s)
		return;
	printf("%s%s%s\n",
	       Color::stdout_success_on, text.c_str(), Color::stdout_success_off);
}

void print_error_silenceable(const char *text)
{
	assert(text && text[0]);
	assert(isupper(text[0]));
	assert(text[strlen(text) - 1] != '\n');
	if (option_s)
		return;
	fprintf(stderr, "%s%s%s\n",
		Color::stderr_err_on, text, Color::stderr_err_off);
}

[[noreturn]]
void error_exit()
{
	terminate_jobs(false);
	exit(ERROR_FATAL);
}
