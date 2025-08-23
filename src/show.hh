#ifndef SHOW_HH
#define SHOW_HH

/*
 * Functions for transforming various objects into string representations.
 * This is a two-step process:
 *
 *              render()                     show()
 *   Objects --------------> Parts -------------------------> Text
 *  (any Stu               (array of                      (std::string
 *   object)              Part objects)                    to be output)
 *
 * render*() functions takes a Rendering parameter, and a "Parts &"
 * parameter to which they append the rendered parts.
 *
 * show*() functions take a Parts object and a Style parameter.
 *
 * There are also show*() functions that take Stu objects and perform both steps
 * at once.
 *
 * A MARKER is an operator that makes unquotes unnecessary in certain contexts.  For
 * instance, a single dependency may be rendered as "A", but a variable dependency as
 * $[A], rather than "A".
 */

/*
 * Show functions are defined in the source files where their datatype is
 * defined.  In classes, they are member functions.
 *
 * Even with S_QUOTE_SOURCE, the resulting strings are not in proper Stu
 * syntax.
 */

#include <assert.h>
#include <string>

#include "color.hh"

typedef unsigned Style;
constexpr Style S_CHANNEL=         (1 << CH_BITS) - 1;
constexpr Style S_ALWAYS_QUOTE=    1 << (CH_BITS + 0);
constexpr Style S_QUOTE_MINIMUM=   1 << (CH_BITS + 1);
constexpr Style S_QUOTE_SOURCE=    1 << (CH_BITS + 2);
constexpr Style S_NO_COLOR=        1 << (CH_BITS + 3);
constexpr Style S_DEFAULT=         CH_ERR;
constexpr Style S_DEBUG=           CH_OUT | S_ALWAYS_QUOTE;
constexpr Style S_NORMAL=          CH_OUT | S_QUOTE_MINIMUM;
constexpr Style S_OPTION_I=        CH_OUT | S_NO_COLOR | S_QUOTE_SOURCE;
constexpr Style S_OPTION_dP=       CH_OUT | S_QUOTE_MINIMUM;

typedef unsigned Rendering;
constexpr Rendering R_SHOW_FLAGS=               1 << 0;
constexpr Rendering R_GLOB=                     1 << 1;
constexpr Rendering R_SHOW_INPUT=               1 << 2;
constexpr Rendering R_NO_COMPOUND_PARENTHESES=  1 << 3;
constexpr Rendering R_SHOW_INDEX=               1 << 4;

enum Properties {
	PROP_TEXT,
	PROP_MARKUP_QUOTABLE,
	PROP_MARKUP_UNQUOTABLE,
	PROP_MARKER,
	PROP_OPERATOR,
	PROP_SPACE,
};

enum Quote_Safeness {
	QS_ALWAYS_QUOTE,
	QS_QUOTE_IN_STU_CODE,
	QS_QUOTE_IN_GLOB_PATTERN,
	QS_SAFE,
	QS_MIN= QS_ALWAYS_QUOTE, QS_MAX= QS_SAFE
};

class Part
{
public:
	Properties properties;
	string text;

	Part(Properties properties_, string text_)
		: properties(properties_), text(text_) { }
	bool is_quotable() const {
		return properties == PROP_TEXT || properties == PROP_MARKUP_QUOTABLE;
	}
	Quote_Safeness need_quotes() const;
	static Quote_Safeness need_quotes(unsigned char);
	void show(string &) const;
	void show(string &, bool quotes) const;
};

class Parts
{
public:
	size_t size() const { return parts.size(); }
	const Part &operator[](size_t i) const { return parts[i]; }
	void append_text(string text) { parts.emplace_back(PROP_TEXT, text); }
	void append_markup_quotable(string op)
	{ parts.emplace_back(PROP_MARKUP_QUOTABLE, op); }
	void append_markup_quotable(char c)
	{ parts.emplace_back(PROP_MARKUP_QUOTABLE, string(1, c)); }
	void append_markup_unquotable(string op)
	{ parts.emplace_back(PROP_MARKUP_UNQUOTABLE, op); }
	void append_markup_unquotable(char c)
	{ parts.emplace_back(PROP_MARKUP_UNQUOTABLE, string(1, c)); }
	void append_marker(string s) {
		assert(s.size() != 0 && s[0] != ' ');
		parts.emplace_back(PROP_MARKER, s);
	}
	void append_operator(string s) {
		assert(s.size() != 0 && s[0] != ' ');
		parts.emplace_back(PROP_OPERATOR, s);
	}
	void append_space() { parts.emplace_back(PROP_SPACE, " "); }
private:
	std::vector <Part> parts;
};

string show(const Parts &, Style style);
void render(string s, Parts &, Rendering= 0);
string show_operator(char, Style= S_DEFAULT);
string show_operator(string, Style= S_DEFAULT);
string show_text(string, Style= S_DEFAULT);
void render_dynamic_variable(string name, Parts &, Rendering= 0);
string show_dynamic_variable(string name, Style= S_DEFAULT);

template <typename T>
void render(const T &, Style= S_DEFAULT, Rendering= 0);
template <typename T>
void render_prefix(string prefix, const T &object, Parts &, Rendering= 0);
template <typename T>
string show_prefix(string prefix, const T &object, Style= S_DEFAULT);
template <typename T>
string show(const T &, Style= S_DEFAULT, Rendering= 0);

#endif /* ! SHOW_HH */
