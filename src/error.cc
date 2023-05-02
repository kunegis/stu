#include "error.hh"

#include <string.h>

#include "color.hh"
#include "format.hh"
#include "options.hh"
#include "text.hh"

void print_error(string message)
{
	assert(! message.empty());
	assert(isupper(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: *** %s\n",
		Color::error_word, dollar_zero, Color::end,
		message.c_str());
}

void print_error_system(string message)
{
	assert(message.size() > 0 && message[0] != '') ;
	string t= name_format_err(message);
	fprintf(stderr, "%s: %s\n", t.c_str(), strerror(errno));
}

void print_error_reminder(string message)
{
	assert(! message.empty());
	assert(isupper(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	fprintf(stderr, "%s%s%s: %s\n",
		Color::warning, dollar_zero, Color::end,
		message.c_str());
}

string system_format(string text)
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
	       Color::out_print, text.c_str(), Color::out_end);
}

void print_error_silenceable(string text)
{
	assert(! text.empty());
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');

	if (option_s)
		return;
	fprintf(stderr,
		"%s%s%s\n",
		Color::error, text.c_str(), Color::end);
}

const Place Place::place_empty;

const Place &Place::operator<<(string message) const
{
	print(message, Color::error, Color::error_word);
	return *this;
}

void Place::print(string message,
		  const char *color,
		  const char *color_word) const
{
	assert(! message.empty());

	switch (type) {
	default:
	case Type::EMPTY:
		/* It's a common bug in Stu to have empty places, so
		 * better provide sensible behavior in NDEBUG builds.  */
		assert(false);
		fprintf(stderr,
			"%s\n",
			message.c_str());
		break;

	case Type::INPUT_FILE:
		assert(line >= 1);
		fprintf(stderr,
			"%s%s%s:%s%zu%s:%s%zu%s: %s\n",
			color_word, get_filename_str(), Color::end,
			color, line, Color::end,
			color, 1 + column, Color::end,
			message.c_str());
		break;

	case Type::ARGUMENT:
		fprintf(stderr,
			"%s%s%s: %s\n",
			color,
			"Command line argument",
			Color::end,
			message.c_str());
		break;

	case Type::OPTION:
		assert(text.size() == 1);
		fprintf(stderr,
			"%sOption %s-%c%s: %s\n",
			color,
			color_word,
			text[0],
			Color::end,
			message.c_str());
		break;

	case Type::ENV_OPTIONS:
		fprintf(stderr,
			"In %s$STU_OPTIONS%s: %s\n",
			color_word, Color::end,
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

bool Place::operator==(const Place &place) const
{
	if (this->type != place.type)
		return false;

	switch (this->type) {
	default:  assert(0);
	case Type::EMPTY:
	case Type::ARGUMENT:
	case Type::ENV_OPTIONS:
		return true;

	case Type::INPUT_FILE:
		return
			this->text == place.text
			&& this->line == place.line
			&& this->column == place.column;

	case Type::OPTION:
		return this->text == place.text;
	}
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
		    Color::warning, Color::warning_word);
}
