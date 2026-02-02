#include "place.hh"

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
		should_not_happen();
		[[fallthrough]];
	case Type::EMPTY:
		fprintf(stderr,
			"%s%s%s: %s\n",
			color_on,
			dollar_zero,
			color_off,
			message.c_str());
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
			color_on,
			dollar_zero,
			color_off,
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
			"%sIn %s$" ENV_STU_OPTIONS "%s%s: %s\n",
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
		should_not_happen();
		[[fallthrough]];
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
	case Type::ARGUMENT: /* uncovered_due_to_bug_in_gcov */
		should_not_happen();
		return "";
	case Type::OPTION:
		return fmt("Option -%s", text);
	case Type::ENV_OPTIONS: /* uncovered_due_to_bug_in_gcov */
		should_not_happen();
		return "$" ENV_STU_OPTIONS;
	}
}

const char *Place::get_filename_str() const
{
	assert(type == Type::INPUT_FILE);
	return text.empty() ? "<stdin>" : text.c_str();
}

void print_warning(const Place &place, string message)
{
	assert(! message.empty());
	assert(islower(message[0]) || message[0] == '\'');
	assert(message[message.size() - 1] != '\n');
	place.print(fmt("warning: %s", message.c_str()),
		    Color::stderr_warn_on, Color::stderr_warn_off);
}
