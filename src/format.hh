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
 * //- format_raw() does not escape anything.
 *
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 */

#include <string>

#include "color.hh"

typedef unsigned Style;
enum
{
	S_OUT=	      1 << 0,
	/* Output for standard output words */
	S_ERR= 	      1 << 1,
	/* Output for standard error output words */
	S_SRC=	      1 << 2,
	/* Output in Stu notation */
//	S_RAW= 	      1 << 3,
//	/* Output in raw form */
	S_CHANNEL=    S_OUT | S_ERR | S_SRC,
	/* Only one of those is set */

	S_INNER=      1 << 3,
	/* Don't output quotes or color codes */
	S_NO_EMPTY=   1 << 4,
	/* Don't need quote around empty content */
	S_NO_FLAGS=   1 << 5,
	/* Do not output flags */
//	S_COLOR_WORD= 1 << 8,
//	S_WANT_ESCAPE=1 << 9, // rename S_ESCAPE
//	/* Characters need to be escaped */
//	S_QUOTES_SINGLE= 1 << 10,
//	S_QUOTES=     S_WANT_ESCAPE | S_QUOTES_SINGLE,
//	S_NEED_QUOTES=1 << 6,
//	/* Out parameter.  Quotes are needed. */
//	/* Used as in/out parameter.  Need quotes around.  Only set if S_ESCAPE
//	 * is set.  */
};

class Quotes
/* The format(...) functions take a Quotes parameter that is a boolean.  This
 * function never outputs surrounding quotes themself, and the parameter is used
 * as both an input and output parameter: as input parameter, it tells the
 * functions whether we are in a context where quotes are needed in principle
 * (because there is no color).  As output parameter, it is set to true when the
 * function detects that quotes are needed.  */
{
public:
	Quotes(Style style)  {
		if (style & S_SRC)
			q= true;
		else if (style & S_OUT)
			q= Color::out_quotes;
		else if (style & S_ERR)
			q= Color::err_quotes;
		else
			assert(false);
	}
	void set()  {  q= true;  }
	bool is() const  {  return q;  }
	
private:
	bool q;
};

string char_format(char c, Style, bool &quotes);
string char_format_err(char c);
string multichar_format_err(string s);
string name_format(string name, Style, Quotes &quotes);
string name_format_err(string name);
string name_format_src(string name, Style= 0);
// XXX is the STYLE parameter really necessary?
string dynamic_variable_format_err(string name);
string prefix_format_err(string name, string prefix);

string quote(string text, Style style, const Quotes &q);
/* Surround by quotes or color codes */

//bool src_need_quotes(const string &name, Style style);
///* Whether a string needs to be quoted in the shell or in Stu (which
// * have the same quoting syntax.)  This is used so that Stu output looks
// * like input to the shell.  Note that for strings beginning with ~ or
// * -, quoting is not enough:  they have to be separated with '--'
// * additionally in the shell.   */

#endif /* ! FORMAT_HH */
