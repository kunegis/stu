#include "color.hh"

#include <string.h>

bool Color::nocolor[CH_COUNT];

const char *Color::stdout_success_on;
const char *Color::stdout_success_off;
const char *Color::stderr_warn_on;
const char *Color::stderr_warn_off;
const char *Color::stderr_err_on;
const char *Color::stderr_err_off;
const char *Color::highlight_on[CH_COUNT];
const char *Color::highlight_off[CH_COUNT];

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

void Color::set(bool enable_color_out, bool enable_color_err)
/* Note:  GCC additionally inserts "\33[K" sequences after each color code, to
 * avoid a bug in some terminals.  This is not done here.  */
{
	if (enable_color_out) {
		nocolor[CH_OUT]= false;
		stdout_success_on=      "\33[32m";
		stdout_success_off=     "\33[m";
		highlight_on[CH_OUT]=   "\33[1m";
		highlight_off[CH_OUT]=  "\33[22m";
	} else {
		nocolor[CH_OUT]= true;
		stdout_success_on=      "";
		stdout_success_off=     "";
		highlight_on[CH_OUT]=   "";
		highlight_off[CH_OUT]=  "";
	}

	if (enable_color_err) {
		nocolor[CH_ERR]= false;
		stderr_warn_on=         "\33[35m";
		stderr_warn_off=        "\33[m";
		stderr_err_on=          "\33[31m";
		stderr_err_off=         "\33[m";
		highlight_on[CH_ERR]=   "\33[1m";
		highlight_off[CH_ERR]=  "\33[22m";
	} else {
		nocolor[CH_ERR]= true;
		stderr_warn_on=         "";
		stderr_warn_off=        "";
		stderr_err_on=          "";
		stderr_err_off=         "";
		highlight_on[CH_ERR]=   "";
		highlight_off[CH_ERR]=  "";
	}
}
