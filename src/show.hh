#ifndef SHOW_HH
#define SHOW_HH

/*
 * Format functions are defined in the source files where their datatype
 * is defined.  In classes, they are member functions.
 */

#include <string>

#include "color.hh"

typedef unsigned Style;
constexpr Style S_CHANNEL=		(1 << CH_BITS) - 1;
constexpr Style S_ALWAYS_QUOTE=		1 << (CH_BITS + 0);
constexpr Style S_QUOTE_MINIMUM=     	1 << (CH_BITS + 1);
constexpr Style S_DEFAULT=              CH_ERR;
constexpr Style S_DEBUG=                CH_OUT | S_ALWAYS_QUOTE;
constexpr Style S_NORMAL=		CH_OUT | S_QUOTE_MINIMUM;

typedef unsigned Rendering;
constexpr Rendering R_SHOW_FLAGS=		1 << 0;
constexpr Rendering R_GLOB=             	1 << 1;
constexpr Rendering R_NO_COMPOUND_PARENTHESES=	1 << 2;

enum Quotable {
	// TODO rename the values to something sensible.
	Q_NO, Q_NO_COLOR, Q_COLOR,
	Q_MIN= Q_NO,
	Q_MAX= Q_COLOR,
};

enum Properties {
	PROP_TEXT, 
	PROP_OPERATOR_QUOTABLE,
	PROP_OPERATOR_UNQUOTABLE,
	PROP_SPACE,
};

class Part
{
public:
	Properties properties;
	string text;

	Part(Properties properties_, string text_)
		:  properties(properties_), text(text_)  {  }
	
	bool is_quotable() const {
		return properties == PROP_TEXT || properties == PROP_OPERATOR_QUOTABLE;
	}
	bool is_operator() const {
		return properties == PROP_OPERATOR_QUOTABLE
			|| properties == PROP_OPERATOR_UNQUOTABLE;
	}
	Quotable need_quotes() const;
	static Quotable need_quotes(unsigned char);
	void show(string &) const;
	void show(string &, bool quotes) const;
};

class Parts
{
public:
	size_t size() const  {  return parts.size();  }
	const Part &operator[](size_t i) const  {  return parts[i];  }
	void append_text(string text)  {  parts.emplace_back(PROP_TEXT, text);  }
	void append_operator_quotable(string op)  {  parts.emplace_back(PROP_OPERATOR_QUOTABLE, op);  }
	void append_operator_quotable(char c)  {  parts.emplace_back(PROP_OPERATOR_QUOTABLE, string(1, c));  }
	void append_operator_unquotable(string op)  {  parts.emplace_back(PROP_OPERATOR_UNQUOTABLE, op);  }
	void append_operator_unquotable(char c)  {  parts.emplace_back(PROP_OPERATOR_UNQUOTABLE, string(1, c));  }
	void append_space()  {  parts.emplace_back(PROP_SPACE, " ");  }
private:
	vector <Part> parts;
};

string show(const Parts &, Style style);
void render(string s, Parts &, Rendering= 0);
string show_operator(char, Style= S_DEFAULT);
string show_operator(string, Style= S_DEFAULT);
string show_text(string, Style= S_DEFAULT);
void render_dynamic_variable(string name, Parts &, Rendering= 0);
string show_dynamic_variable(string name, Style= S_DEFAULT);

template <typename T>
void render_prefix(string prefix, const T &object, Parts &, Rendering= 0);
template <typename T>
string show_prefix(string prefix, const T &object, Style= S_DEFAULT);
template <typename T>
string show(const T &, Style= S_DEFAULT, Rendering= 0);

#endif /* ! SHOW_HH */
