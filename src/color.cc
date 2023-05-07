#include "color.hh"

#include <string.h>

bool Color::err_quotes, Color::out_quotes;

const char *Color::end;
const char *Color::error;
const char *Color::warning;
const char *Color::word;
const char *Color::error_word;
const char *Color::warning_word;

const char *Color::out_end;
const char *Color::out_print_word_end;
const char *Color::out_print;
const char *Color::out_print_word;

void Color::set()
/* Only use color when $TERM is defined, is not equal to "dumb", and
 * stderr/stdout is a TYY.  This is  the same logic as used by GCC.  */
{
	bool is_tty_out= false, is_tty_err= false;
	const char *t= getenv("TERM");

	if (t && strcmp(t, "dumb")) {
		errno= 0;
		is_tty_out= isatty(fileno(stdout));
		if (! is_tty_out && errno != 0 && errno != ENOTTY) {
			perror("isatty");
		}
		errno= 0;
		is_tty_err= isatty(fileno(stderr));
		if (! is_tty_err && errno != 0 && errno != ENOTTY) {
			perror("isatty");
		}
	}

	set(is_tty_out, is_tty_err);
}

//void Color::set(bool enable_color)
//{
//	set(enable_color, enable_color);
//}

void Color::set(bool enable_color_out, bool enable_color_err)
/* Note:  GCC additionally inserts "\33[K" sequences after each color code, to
 * avoid a bug in some terminals.  This is not done here.  */
{
	if (enable_color_out) {
		out_quotes= false;
		out_end=            "\33[0m";
		out_print_word_end= "\33[0;32m";
		out_print=          "\33[32m";
		out_print_word=     "\33[32;1m";
	} else {
		out_quotes= true;
		out_end=            "";
		out_print_word_end= "";
		out_print=          "";
		out_print_word=     "";
	}

	if (enable_color_err) {
		err_quotes= false;
		error=              "\33[31m";
		warning=            "\33[35m";
		word=               "\33[1m";
		error_word=         "\33[1;31m";
		warning_word=       "\33[1;35m";
		end=                "\33[0m";
	} else {
		err_quotes= true;
		error=              "";
		warning=            "";
		word=               "";
		error_word=         "";
		warning_word=       "";
		end=                "";
	}
}
