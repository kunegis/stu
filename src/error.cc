#include "error.hh"

#include <string.h>

#include "color.hh"
#include "format.hh"
#include "options.hh"
#include "show.hh"

void print_error(string message)
{
	assert(! message.empty());
	assert(isupper(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: *** %s\n",
		Color::stderr_err_on, dollar_zero, Color::stderr_err_off,
		message.c_str());
}

void print_errno(string message)
{
	// TODO use color
	assert(message.size() > 0 && message[0] != '') ;
	fprintf(stderr, "%s: %s\n", message.c_str(), strerror(errno));
}

void print_error_reminder(string message)
{
	assert(! message.empty());
	assert(isupper(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: %s\n",
		Color::stderr_warn_on, dollar_zero, Color::stderr_warn_off,
		message.c_str());
}

string format_errno(string text)
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

void print_error_silenceable(string text)
{
	assert(! text.empty());
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');
	if (option_s)
		return;
	fprintf(stderr, "%s%s%s\n",
		Color::stderr_err_on, text.c_str(), Color::stderr_err_off);
}

const Place Place::place_empty;

const Place &Place::operator<<(string message) const
{
	print(message, Color::stderr_err_on, Color::stderr_err_off);
	return *this;
}

void Place::print(string message,
		  const char *color_on,
		  const char *color_off) const
{
	assert(! message.empty());

	switch (type) {
	default:
	case Type::EMPTY:
		/* It's a common bug in Stu to have empty places, so
		 * better provide sensible behavior in NDEBUG builds.  */
		assert(false);
		fprintf(stderr, "%s\n", message.c_str());
		break;

	case Type::INPUT_FILE:
		assert(line >= 1);
		fprintf(stderr,
			"%s%s%s%s:%zu:%zu%s: %s\n",
			color_on,
			Color::highlight_on[CH_ERR],
			get_filename_str(),
			Color::highlight_off[CH_ERR],
			line,
			1 + column,
			color_off,
			message.c_str());
		break;

	case Type::ARGUMENT:
		fprintf(stderr,
			"%s%s%s: %s\n",
			color_on, "Command line argument", color_off,
			message.c_str());
		break;

	case Type::OPTION:
		assert(text.size() == 1);
		fprintf(stderr,
			"%sOption %s-%c%s%s: %s\n",
			color_on,
			Color::highlight_on[CH_ERR],
			text[0],
			Color::highlight_off[CH_ERR],
			color_off,
			message.c_str());
		break;

	case Type::ENV_OPTIONS:
		fprintf(stderr,
			"%sIn %s$STU_OPTIONS%s%s: %s\n",
			color_on, Color::highlight_on[CH_ERR],
			Color::highlight_off[CH_ERR], color_off,
			message.c_str());
		break;
	}
}

string Place::as_argv0() const
{
	switch (type) {
	default:
	case Type::EMPTY:
	case Type::ENV_OPTIONS:
		assert(false);
	case Type::ARGUMENT:
		return "";
	case Type::OPTION:
		return fmt("Option -%s", text);
	case Type::INPUT_FILE: {
		/* The given argv[0] should not begin with a dash,
		 * because some shells enable special behaviour
		 * (restricted/login mode and similar) when argv[0]
		 * begins with a dash. */
		const char *s= get_filename_str();
		return frmt("%s%s:%zu",
			    s[0] == '-' ? "file " : "",
			    s, line);
	}
	}
}

const char *Place::get_filename_str() const
{
	assert(type == Type::INPUT_FILE);
	return text.empty() ? "<stdin>" : text.c_str();
}

bool Place::operator<(const Place &place) const
{
	if (this->type != place.type) {
		return this->type < place.type;
	}
	switch (this->type) {
	default:  assert(0);
	case Type::EMPTY:
	case Type::ARGUMENT:
	case Type::ENV_OPTIONS:
		return false;
	case Type::INPUT_FILE:
		if (this->text != place.text)
			return this->text < place.text;
		if (this->line != place.line)
			return this->line < place.line;
		return this->column < place.column;
	case Type::OPTION:
		return this->text < place.text;
	}
}

void print_warning(const Place &place, string message)
{
	assert(! message.empty());
	assert(isupper(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	place.print(fmt("warning: %s", message),
		    Color::stderr_warn_on, Color::stderr_warn_off);
}
