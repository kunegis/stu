#ifndef FORMAT_HH
#define FORMAT_HH

// TODO rename format to "show" to avoid clash with the format functions
// in text.hh and also with the upcoming C++20 ones.

/*
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 */

#include <string>

#include "color.hh"

typedef unsigned Show_Bits;
constexpr Show_Bits S_STDOUT=	           1 << 0;
constexpr Show_Bits S_STDERR= 	           1 << 1;
constexpr Show_Bits S_DONT_SHOW_COLOR=     1 << 2;
constexpr Show_Bits S_DONT_SHOW_QUOTES=    1 << 3;
constexpr Show_Bits S_SHOW_FLAGS=          1 << 4;
constexpr Show_Bits S_NEED_QUOTES=         1 << 5;
constexpr Show_Bits S_DONT_FORMAT=         1 << 6;
constexpr Show_Bits S_NO_EMPTY=            1 << 7;
constexpr Show_Bits S_HAS_MARKER=          1 << 8;
constexpr Show_Bits S_GLOB=                1 << 9;
constexpr Show_Bits S_DEFAULT=             S_STDERR;
constexpr Show_Bits S_DEBUG=               S_STDOUT | S_DONT_SHOW_COLOR;

/*
// * If S_DONT_SHOW_QUOTES is set, caller must show quotes when S_NEED_QUOTES is
// * set. 
 *
 * S_NEED_QUOTES is used as an in/out parameter.  The others only as in
 * parameter. 
 */

class Style
{
public:
	Style(Show_Bits bits_);
	
	Show_Bits operator&(Show_Bits b) const  {  return bits & b;  }
	bool is() const  {  return bits & S_NEED_QUOTES;  }

	// TODO maybe deprecate, because it is an error to set certain bits
	// after the constructor.  Instead, callers must pass the bits in the constructor.
	void operator|=(Show_Bits b) {
		bits |= b;
		check(); 
	}

	void set()  {
		bits |= S_NEED_QUOTES;
		check(); 
	}
	
	void check() const {
		assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1); 
	}

	static Style inner(const Style *parent,
			   bool may_show_quotes,
			   Show_Bits other_bits= 0); 

	static Style outer(const Style *style, const Style *style_inner);
	/* If STYLE_INNER is false, assume all inner parts were shown with
	 * quotes.  */

private:
	Show_Bits bits;
	void init();
};

string show(string text, Style *style= nullptr); 
string show_dynamic_variable(string name, Style *style= nullptr);

string show(char c, Style *style= nullptr) {
	return show(string(1, c), style); 
}

template <typename T>
string show_prefix(string prefix, const T &object, Style *style= nullptr)
{
	Style style_inner= Style::inner(style, true, S_HAS_MARKER);
	string s= show(object, &style_inner);
	string ret= prefix + s;
	Style style_outer= Style::outer(style, &style_inner);
	return show(ret, &style_outer); 
}

#endif /* ! FORMAT_HH */
