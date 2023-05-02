#ifndef FORMAT_HH
#define FORMAT_HH

/*
 * - format() formats the content according to the exact specification, but
 *   never surrounds it by quotes.  It may include more parameters to configure
 *   the output.
 * - format_err() returns a string suitable for inclusion in a message
 *   on STDERR, including quotes and color, as appropriate.  It does not
 *   show flags.
 * - format_out() returns the same as format_err(), but for STDOUT.
 * - format_src() formats an expression as if it was part of the source,
 *   e.g., use quotes only if the name contains characters that need to
 *   be quoted.  It does include the flags.
 * - raw() does not escape anything.
 *
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 *
 * The format(...) functions take a QUOTE parameter that is a reference
 * to a boolean.  This function never outputs surrounding quotes
 * themself, and the QUOTES parameter is used as both an input and
 * output parameter: as input parameter, it tells the functions whether
 * we are in a context where quotes are needed in principle (because
 * there is no color).  As output parameter, it is set to true when the
 * function detects that quotes are needed.
 */

#include <string>

typedef unsigned Style;
enum
{
	S_MARKERS=    1 << 0,
	/* There will be markers around or to the left of the text */

	S_NOEMPTY=    1 << 1,
	/* Don't need quote around empty content */

	S_NOFLAGS=    1 << 2,
	/* Do not output flags; normally used with S_OUT and S_ERR */

	S_OUT=	      1 << 3,
	/* Output for standard output words */

	S_ERR= 	      1 << 4,
	/* Output for standard error output words */

	S_SRC=	      1 << 5,
	/* Output in Stu notation */

	S_RAW= 	      1 << 6,
	/* Output in raw form */

	S_COLOR_WORD= 1 << 7,

	S_CHANNEL=    S_OUT | S_ERR | S_SRC | S_RAW,
	/* Only one of those is set */
};

string char_format(char c, Style style, bool &quotes);
string char_format_err(char c);
string multichar_format_err(string s);
string name_format(string name, Style style, bool &quotes);
string name_format_err(string name);
string name_format_src(string name);
string dynamic_variable_format_err(string name);
string prefix_format_err(string name, string prefix);

bool src_need_quotes(const string &name);
/* Whether a string needs to be quoted in the shell or in Stu (which
 * have the same quoting syntax.)  This is used so that Stu output looks
 * like input to the shell.  Note that for strings beginning with ~ or
 * -, quoting is not enough:  they have to be separated with '--'
 * additionally in the shell.   */

#endif /* ! FORMAT_HH */
