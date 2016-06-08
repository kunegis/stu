#ifndef COLOR_HH
#define COLOR_HH

/* Handling of color output.  All color output is done on standard error
 * output.   
 *
 * Colors:  
 *
 *   error:  the filename or argument of error messages
 *   warning:the filename or argument of warning messages
 *   name:   names quoting the input 
 *   error name:  name inside an error
 */

/* ANSI escape codes are explained in the man page console_codes(4) on
 * Linux.  What we use:
 *
 *  0: reset
 *  1: bright/bold
 * 31: red
 * 35: magenta
 *
 * Colors and styles look different in different terminals.  In
 * particular in black-on-white vs white-on-black terminals. 
 */

class Color 
{
public:

	/* Whether the texts are already color quoted */
	static bool has_quotes;

	static const char *beg_error,          *end_error;
	static const char *beg_warning,        *end_warning;
	static const char *beg_name_bare,      *end_name_bare;
	static const char *beg_name_quoted,    *end_name_quoted;
	static const char *beg_error_name,     *end_error_name;

	static void set(bool is_tty);

private:

	static Color color;

	Color() {
		errno= 0;
		bool is_tty= isatty(fileno(stderr)); 
		if (errno == EBADF) {
			perror("isatty");
		}
		set(is_tty); 
	}	
};

Color Color::color;

bool Color::has_quotes;

const char *Color::beg_error;
const char *Color::end_error;
const char *Color::beg_warning;
const char *Color::end_warning;
const char *Color::beg_name_bare;
const char *Color::end_name_bare;
const char *Color::beg_name_quoted;
const char *Color::end_name_quoted;
const char *Color::beg_error_name;
const char *Color::end_error_name;

void Color::set(bool is_tty)
{
	if (is_tty) {
		has_quotes= true;
		beg_error=        "[31m";
		end_error=        "[0m";
		beg_warning=      "[35m";
		end_warning=      "[0m";
		beg_name_bare=    "[1m";
		end_name_bare=    "[0m";
		beg_name_quoted=  "[1m"; 
		end_name_quoted=  "[0m";
		beg_error_name=   "[1;31m"; 
		end_error_name=   "[0m";
	} else {
		has_quotes= false;
		beg_error=        "";
		end_error=        "";
		beg_warning=      "";
		end_warning=      "";
		beg_name_bare=    ""; 
		end_name_bare=    "";
		beg_name_quoted=  "'"; 
		end_name_quoted=  "'";
		beg_error_name=   ""; 
		end_error_name=   "";
	}
}

#endif /* ! COLOR_HH */
