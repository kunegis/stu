#ifndef SHOW_HH
#define SHOW_HH

/*
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 */

#include <string>

#include "color.hh"
#include "tracing.hh"

typedef unsigned Style_Bits;

constexpr Style_Bits S_STDOUT=	           	1 <<  0;
constexpr Style_Bits S_STDERR= 	                1 <<  1;
constexpr Style_Bits S_DONT_SHOW_COLOR=         1 <<  2;
constexpr Style_Bits S_QUOTES_MAY_INHERIT_UP=	1 <<  3;
constexpr Style_Bits S_QUOTES_INHERIT_UP=       1 <<  4;
constexpr Style_Bits S_SHOW_FLAGS=              1 <<  5;
constexpr Style_Bits S_NEED_QUOTES_NOCOLOR= 	1 <<  6; /* Need quotes because color is not used */
constexpr Style_Bits S_NEED_QUOTES_CHAR=    	1 <<  7; /* Need quotes because there is a special character */
constexpr Style_Bits S_OUTER=	         	1 <<  8;
constexpr Style_Bits S_NO_EMPTY=            	1 <<  9;
constexpr Style_Bits S_HAS_MARKER=          	1 << 10;
constexpr Style_Bits S_GLOB=                	1 << 11;

constexpr Style_Bits S_DEFAULT=             	S_STDERR;
constexpr Style_Bits S_DEBUG=               	S_STDOUT | S_DONT_SHOW_COLOR;

/*
 * S_NEED_QUOTES is used as an in/out parameter.  The others only as in
 * parameter. 
 */

class Style
{
public:
	friend string style_format(const Style *style); 

	Style(Style_Bits bits_): bits(bits_)  {  check(); init(true);  }
	Style(Style_Bits bits_, bool toplevel);
	
	Style_Bits operator&(Style_Bits b) const  {  return bits & b;  }

	// TODO maybe deprecate, because it is an error to set certain bits
	// after the constructor.  Instead, callers must pass the bits in the constructor.
	void operator|=(Style_Bits b)  {  bits |= b;  check();  }

	void set()  {  bits |= S_NEED_QUOTES_CHAR;  check();  }
	bool is() const   {  return bits & S_NEED_QUOTES_CHAR;  }
	
	void check() const {
		assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1); 
	}

	static Style inner(const Style *parent, Style_Bits other_bits= 0); 

	static Style outer(const Style *style, const Style *style_inner,
			   Style_Bits other_bits= 0);
	/* If STYLE_INNER is false, assume all inner parts were shown with
	 * quotes.  */

private:
	Style_Bits bits;
	void init(bool toplevel);
};

string show(string text, Style *style= nullptr); 
string show_dynamic_variable(string name, Style *style= nullptr);
string show_operator(char c);
string show_operator(string s);

template <typename T>
string show_prefix(string prefix, const T &object, Style *style= nullptr);

// TODO put into .cc
string style_format(const Style *style)
{
	if (style == nullptr)
		return "<null>";

	string ret= "";

	if (style->bits & S_STDOUT)
		ret += "STDOUT ";
	if (style->bits & S_STDERR)
		ret += "STDERR ";
	if (style->bits & S_DONT_SHOW_COLOR)
		ret += "DONT_SHOW_COLOR ";
	if (style->bits & S_QUOTES_MAY_INHERIT_UP)
		ret += "QUOTES_MAY_INHERIT_UP ";
	if (style->bits & S_QUOTES_INHERIT_UP)
		ret += "QUOTES_INHERIT_UP ";
	if (style->bits & S_SHOW_FLAGS)
		ret += "SHOW_FLAGS ";
	if (style->bits & S_NEED_QUOTES_NOCOLOR)
		ret += "NEED_QUOTES_NOCOLOR ";
	if (style->bits & S_NEED_QUOTES_CHAR)
		ret += "NEED_QUOTES_CHAR ";
	if (style->bits & S_OUTER)
		ret += "OUTER ";
	if (style->bits & S_NO_EMPTY)
		ret += "NO_EMPTY ";
	if (style->bits & S_HAS_MARKER)
		ret += "HAS_MARKER ";
	if (style->bits & S_GLOB)
		ret += "GLOB ";

	return ret;
}

#endif /* ! SHOW_HH */
