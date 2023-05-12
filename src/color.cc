#include "color.hh"

#include <string.h>

bool Color::err_quotes, Color::out_quotes;

const char *Color::stdout_success_on;
const char *Color::stdout_success_off;   
const char *Color::stdout_highlight_on;    
const char *Color::stdout_highlight_off;  

const char *Color::stderr_warn_on;        
const char *Color::stderr_warn_off;       
const char *Color::stderr_err_on;         
const char *Color::stderr_err_off;        
const char *Color::stderr_highlight_on;    
const char *Color::stderr_highlight_off;  

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
	// TODO order should be the same as in declaration. 
	if (enable_color_out) {
		out_quotes= false;
		stdout_success_on=    "\33[32m";
		stdout_success_off=   "\33[m";
		stdout_highlight_on=  "\33[1m";
		stdout_highlight_off= "\33[22m";
	} else {
		out_quotes= true;
		stdout_success_on=    "";
		stdout_success_off=   "";
		stdout_highlight_on=  "";
		stdout_highlight_off= "";
	}

	if (enable_color_err) {
		err_quotes= false;
		stderr_warn_on=       "\33[35m";
		stderr_warn_off=      "\33[m";
		stderr_err_on=        "\33[31m";
		stderr_err_off=       "\33[m";
		stderr_highlight_on=  "\33[1m";
		stderr_highlight_off= "\33[22m";
	} else {
		err_quotes= true;
		stderr_warn_on=       "";
		stderr_warn_off=      "";
		stderr_err_on=        "";
		stderr_err_off=       "";
		stderr_highlight_on=  "";
		stderr_highlight_off= "";
	}
}
