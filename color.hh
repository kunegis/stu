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

/*
 * To cite the file gcc/diagnostic-color.c in gcc-6.2.0:
 *
 *     "It would be impractical for GCC to become a full-fledged terminal
 *     program linked against ncurses or the like, so it will not detect
 *     terminfo(5) capabilities."
 *
 * Stu takes the same approach. 
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
		
		/* 
		 * Logic:  Only use color when $TERM is defined, is not
		 * equal to "dumb", and stderr/stdout is a TYY.  This is
		 * the same logic used by GCC. 
		 */

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
	/*
	 * Note:  GCC addition inserts "\33[K" sequences after each
	 * color code, to avoid a bug in some terminals.  This is not
	 * done here. 
	 */

	if (is_tty_out) {
		quotes_out= false;
		out_end=            "\33[0m";
		out_print_word_end= "\33[0;32m"; 
		out_print=          "\33[32m";
		out_print_word=     "\33[32;1m"; 
	} else {
		quotes_out= true;
		out_end=            ""; 
		out_print_word_end= "";
		out_print=          "";
		out_print_word=     "";
	}

	if (is_tty_err) {
		quotes= false;
		error=             "\33[31m";
		warning=           "\33[35m";
		word=              "\33[1m";
		error_word=        "\33[1;31m"; 
		warning_word=      "\33[1;35m"; 
		end=               "\33[0m";
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
