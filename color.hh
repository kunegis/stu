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

	/* Whether single quotes have to be used.  Only true when color
	 * is not used.  */   
	static bool quotes; 

	static const char *error;
	static const char *warning;
	static const char *word;
	static const char *error_word;
	static const char *end;

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

bool Color::quotes;

const char *Color::error;
const char *Color::warning;
const char *Color::word;
const char *Color::error_word;
const char *Color::end;

void Color::set(bool is_tty)
{
	if (is_tty) {
		quotes= false;
		error=             "[31m";
		warning=           "[35m";
		word=              "[1m";
		error_word=        "[1;31m"; 
		end=               "[0m";
	} else {
		quotes= true;
		error=        "";
		warning=      "";
		word=         ""; 
		error_word=   ""; 
		end=          "";
	}
}

#endif /* ! COLOR_HH */
