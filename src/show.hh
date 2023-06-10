#ifndef SHOW_HH
#define SHOW_HH

/*
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 */

#include <string>

#include "color.hh"
#include "tracing.hh"

typedef unsigned Style;
constexpr Style S_STDOUT=		1 << 0;
constexpr Style S_STDERR=		1 << 1;
constexpr Style S_ALWAYS_QUOTE=		1 << 2;
constexpr Style S_QUOTE_MINIMUM=     	1 << 3;
constexpr Style S_DEFAULT=              S_STDERR;
constexpr Style S_DEBUG=                S_STDOUT | S_ALWAYS_QUOTE;
constexpr Style S_NORMAL=		S_STDOUT | S_QUOTE_MINIMUM;

typedef unsigned Rendering;
constexpr Rendering R_SHOW_FLAGS=	1 << 0;

// constexpr Style_Bits S_STDOUT=	           	1 <<  0;
// constexpr Style_Bits S_STDERR= 	                1 <<  1;
// constexpr Style_Bits S_DONT_SHOW_COLOR=         1 <<  2;
// constexpr Style_Bits S_QUOTES_MAY_INHERIT_UP=	1 <<  3;
// constexpr Style_Bits S_QUOTES_INHERIT_UP=       1 <<  4;
// constexpr Style_Bits S_SHOW_FLAGS=              1 <<  5;
// constexpr Style_Bits S_NEED_QUOTES_NOCOLOR= 	1 <<  6; /* Need quotes because color is not used */
// constexpr Style_Bits S_NEED_QUOTES_CHAR=    	1 <<  7; /* Need quotes because there is a special character */
// constexpr Style_Bits S_OUTER=         		1 <<  8;
// constexpr Style_Bits S_NO_EMPTY=            	1 <<  9;
// constexpr Style_Bits S_HAS_MARKER=          	1 << 10;
// constexpr Style_Bits S_GLOB=                	1 << 11;

// constexpr Style_Bits S_DEFAULT=             	S_STDERR;
// constexpr Style_Bits S_DEBUG=               	S_STDOUT | S_NEED_QUOTES_NOCOLOR;

///*
// * S_NEED_QUOTES is used as an in/out parameter.  The others only as in
// * parameter. 
// */

// class Style
// {
// public:
// 	friend string style_format(const Style *style); 

// 	Style(Style_Bits bits_): bits(bits_)  {  check(); init(true);  }
// 	Style(Style_Bits bits_, bool toplevel);
	
// 	Style_Bits operator&(Style_Bits b) const  {  return bits & b;  }

// 	// TODO maybe deprecate, because it is an error to set certain bits
// 	// after the constructor.  Instead, callers must pass the bits in the constructor.
// 	void operator|=(Style_Bits b)  {  bits |= b;  check();  }

// 	void set()  {  bits |= S_NEED_QUOTES_CHAR;  check();  }
// 	bool is() const   {  return bits & S_NEED_QUOTES_CHAR;  }
	
// 	void check() const {
// 		assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1); 
// 	}

// 	static Style inner(const Style *parent, Style_Bits other_bits= 0); 

// 	static Style outer(const Style *style, const Style *style_inner,
// 			   Style_Bits other_bits= 0);

// 	static void transfer(Style *style, const Style *style_outer);

// private:
// 	Style_Bits bits;
// 	void init(bool toplevel);
// };

enum Properties {
	PROP_TEXT, 
	PROP_OPERATOR,
	PROP_SPACE,
};

class Part
{
public:
	Properties properties;
	string text;
};

class Parts
{
public:
	void append_operator(string op)  {  parts.emplace_back(PROP_OPERATOR, op);  }
	void append_operator(char c)  {  parts.emplace_back(PROP_OPERATOR, string(c, 1));  }
	void append_space()  {  parts.emplace_back(PROP_SPACE, " ");  }
private:
	vector <Part> parts;
};

string show(const Parts &);

//void render_operator(char, Parts &, Rendering= 0);
//void render_operator(string, Parts &, Rendering= 0);
string show_operator(char, Style= S_DEFAULT);
string show_operator(string, Style= S_DEFAULT);
void render_dynamic_variable(string name, Parts &, Rendering= 0);
string show_dynamic_variable(string name, Style= S_DEFAULT);

template <typename T> void render_prefix(Parts &, string prefix, const T &object);
template <typename T> string show_prefix(string prefix, const T &object, Style= S_DEFAULT);

template <typename T>
string show(const T &, Style= S_DEFAULT);

string style_format(Style);

#endif /* ! SHOW_HH */
