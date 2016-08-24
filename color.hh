#ifndef COLOR_HH
#define COLOR_HH

/* 
 * Handling of color output.  
 *
 * Colors:  
 *
 *   error:       The filename or argument of error messages
 *   warning:     The filename or argument of warning messages
 *   name:        Names quoting the input 
 *   error name:  Name inside an error
 */

/* 
 * ANSI escape codes are explained in the man page console_codes(4) on
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

	/* Whether single quotes have to be used.  Only true when color
	 * is not used.  */   
	static bool quotes, quotes_out; 

	static const char *end;
	static const char *error;
	static const char *warning;
	static const char *word;
	static const char *error_word;
	static const char *warning_word;

	static const char *out_end; 
	static const char *out_print_word_end;
	static const char *out_print;
	static const char *out_print_word;

	static void set(bool is_tty_out, bool is_tty_err);

private:

	static Color color;

	Color() {
		errno= 0;
		bool is_tty_out= isatty(fileno(stdout)); 
		if (! is_tty_out && errno != 0 && errno != ENOTTY) {
			perror("isatty"); 
		}
		errno= 0;
		bool is_tty_err= isatty(fileno(stderr)); 
		if (! is_tty_err && errno != 0 && errno != ENOTTY) {
			perror("isatty");
		}
		set(is_tty_out, is_tty_err); 
	}	
};

bool Color::quotes, Color::quotes_out;

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

Color Color::color;

void Color::set(bool is_tty_out, bool is_tty_err)
{
	if (is_tty_out) {
		quotes_out= false;
		out_end=            "[0m";
		out_print_word_end= "[0;32m"; 
		out_print=          "[32m";
		out_print_word=     "[32;1m"; 
	} else {
		quotes_out= true;
		out_end=            ""; 
		out_print_word_end= "";
		out_print=          "";
		out_print_word=     "";
	}

	if (is_tty_err) {
		quotes= false;
		error=             "[31m";
		warning=           "[35m";
		word=              "[1m";
		error_word=        "[1;31m"; 
		warning_word=      "[1;35m"; 
		end=               "[0m";
	} else {
		quotes= true;
		error=        "";
		warning=      "";
		word=         ""; 
		error_word=   ""; 
		warning_word= ""; 
		end=          "";
	}
}

#endif /* ! COLOR_HH */
