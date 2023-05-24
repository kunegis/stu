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
constexpr Show_Bits S_GLOB=                1 << 8;
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
	// TODO replace the two constructors by functions returning a Style that
	// can be assigned to another Style (which will need a copy constructor). 
	
	Style(Show_Bits bits_);
	Style(Style *style, bool may_show_quotes);  /* Inner style */

	Style(const Style *style, const Style *style_inner, int);
	/* Outer style.  If STYLE_INNER is false, assume all inner parts were
	 * shown with quotes. */
	
	Show_Bits operator&(Show_Bits b) const  {  return bits & b;  }
	void operator|=(Show_Bits b)  {  bits |= b;  }
	void set()  {  bits |= S_NEED_QUOTES;  }
	bool is() const  {  return bits & S_NEED_QUOTES;  }
	
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
string show_prefix(string prefix, const T &object, Style *style= nullptr) {
	Style style_inner(style, true);
	string s= show(object, &style_inner);
	string ret= prefix + s;
	Style style_outer(style, &style_inner, 0);
	return show(ret, &style_outer); 
}

#endif /* ! FORMAT_HH */
