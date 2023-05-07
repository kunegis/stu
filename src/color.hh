#ifndef COLOR_HH
#define COLOR_HH

/*
 * Handling of color output.
 *
 * Colors:
 *   error:          The place of an error
 *   warning:        The place of a warning
 *   word:           Names quoting the input
 *   error word:     Name inside an error place
 *   warning word:   Name inside a warning place
 */

/*
 * We use ANSI escape codes to display color.  ANSI escape codes are
 * explained in the man page console_codes(4) on Linux.  What we use:
 *
 *  0: reset
 *  1: bright/bold        (for names)
 * 31: red                (for errors)
 * 32: green              (for success)
 * 35: magenta            (for warnings)
 *
 * Colors and styles look different in different terminals.  In
 * particular in black-on-white vs white-on-black terminals.
 */

/*
 * To cite the file gcc/diagnostic-color.c in gcc-6.2.0:
 *
 *     "It would be impractical for GCC to become a full-fledged terminal
 *      program linked against ncurses or the like, so it will not detect
 *      terminfo(5) capabilities."
 *
 * Stu takes the same approach.
 */

class Color
{
public:
	static bool err_quotes, out_quotes;
	/* Whether single quotes have to be used.  Set when color is not used. */

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

	/* At least one of the following functions must be called before
	 * any output is written.  */
	static void set();
//	static void set(bool enable_color);
	static void set(bool enable_color_out, bool enable_color_err);
};

#endif /* ! COLOR_HH */
