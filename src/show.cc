#include "show.hh"

Style::Style(Style_Bits bits_, bool toplevel)
	:  bits(bits_)
{
	init(toplevel); 
}

void Style::init(bool toplevel)
{
	if (toplevel) {
		if (bits & S_STDOUT) {
			if (Color::out_quotes)
				bits |= S_NEED_QUOTES_NOCOLOR;
		} else if (bits & S_STDERR) {
			if (Color::err_quotes) 
				bits |= S_NEED_QUOTES_NOCOLOR;
		} else {
			assert(false);
		}
	}
	check(); 
}

Style Style::inner(const Style *parent,
		   Style_Bits other_bits)
{
	Style_Bits bits= S_DONT_SHOW_COLOR | other_bits;
	if (parent) {
		bits |= (parent->bits & ~(S_QUOTES_MAY_INHERIT_UP));
	} else {
		bits |= S_DEFAULT;
	}
	return Style(bits, !parent);
}

Style Style::outer(const Style *parent, const Style *style_inner,
		   Style_Bits other_bits)
{
	TRACE_FUNCTION(SHOW, Style::outer);
	TRACE("style_inner= %s", style_format(style_inner));

	Style_Bits bits= S_OUTER | other_bits;
	if (parent) {
		bits |= parent->bits;
	} else {
		bits |= S_DEFAULT;
	}
	bits &= ~S_NEED_QUOTES_NOCOLOR;
	if (style_inner && style_inner->bits & S_QUOTES_INHERIT_UP) {
		bits |= S_NEED_QUOTES_CHAR;
	}
	Style ret= Style(bits, !parent);
	TRACE("ret= %s", style_format(&ret));
	return ret;
}

string show(string name, Style *style)
{
	TRACE_FUNCTION(SHOW, show(string)); 
	TRACE("(%s) %s", name, style_format(style));

	Style a_style= S_DEFAULT;
	if (!style) {
		style= &a_style;
		TRACE("default style= %s", style_format(style));
	}
	style->check(); 
	
	if (name.empty() && ! (*style & S_NO_EMPTY)) {
		style->set();
		TRACE("%s", "set because name is empty");
	}

	for (size_t i= 0;  !(*style & S_OUTER) && !style->is() && i < name.size();  ++i) {
		char c= name[i];
		if (c >= 0 && c <= ' ' || c == 0x7F) {
			style->set();
			TRACE("set because name name contains character %s that needs escape", frmt("%d", c));
		}
	}

	string ret(4 * name.size(), '\0');
	char *const p_begin= &ret[0], *p= p_begin;
	bool actually_need_quotes_nocolor= ((*style & S_NEED_QUOTES_NOCOLOR) && !(*style & S_HAS_MARKER) && !(*style & S_OUTER));
	if (*style & S_OUTER) {
		ret= name;
	} else {
		for (size_t i= 0;  i < name.size();  ++i) {
			char c= name[i];
			unsigned char cu= (unsigned char) c;
			if (c == ' ') {
				*p++= ' ';
				// TODO contract the next three cases
			} else if (c == '\\') {
				if (style->is() || actually_need_quotes_nocolor)
					*p++= '\\';
				*p++= '\\';
			} else if (c == '\"') {
				if (style->is() || actually_need_quotes_nocolor)
					*p++= '\\';
				*p++= '\"';
			} else if (c == '$') {
				if (style->is() || actually_need_quotes_nocolor)
					*p++= '\\';
				*p++= '$';
			} else if (c == '\0') {
				*p++= '\\';
				*p++= '0';
			} else if (c == '\a') {
				*p++= '\\';
				*p++= 'a';
			} else if (c == '\b') {
				*p++= '\\';
				*p++= 'b';
			} else if (c == '\f') {
				*p++= '\\';
				*p++= 'f';
			} else if (c == '\n') {
				*p++= '\\';
				*p++= 'n';
			} else if (c == '\r') {
				*p++= '\\';
				*p++= 'r';
			} else if (c == '\t') {
				*p++= '\\';
				*p++= 't';
			} else if (c == '\v') {
				*p++= '\\';
				*p++= 'v';
			} else if (cu >= 0x21 && cu != 0x7F) {
				*p++= c;
			} else {
				*p++= '\\';
				*p++= 'x';
				*p++= "0123456789ABCDEF"[cu/16];
				*p++= "0123456789ABCDEF"[cu%16];
			}
		}
		ret.resize(p - p_begin);
	}

	bool quotes= style->is() || actually_need_quotes_nocolor;
	if (quotes && *style & S_QUOTES_MAY_INHERIT_UP) {
		quotes= false;
		*style |= S_QUOTES_INHERIT_UP;
		TRACE("%s", "inherit up");
	}
	if (quotes) {
		ret= fmt("\"%s\"", ret);
	}

	if (*style & S_DONT_SHOW_COLOR) {
		/* noop */
	} else if (*style & S_STDOUT) {
		ret= fmt("%s%s%s", Color::stdout_highlight_on,
			   ret, Color::stdout_highlight_off); 
	} else if (*style & S_STDERR) {
		ret= fmt("%s%s%s", Color::stderr_highlight_on,
			   ret, Color::stderr_highlight_off);
	} else
		assert(false);

	TRACE("style[out]= %s", style_format(style));
	TRACE("ret= %s", ret);
	return ret; 
}

string show_dynamic_variable(string name, Style *style)
{
	Style style_inner= Style::inner(style, S_QUOTES_MAY_INHERIT_UP);
	string s= show(name, &style_inner);
	string ret= fmt("$[%s]", s);
	Style style_outer= Style::outer(style, &style_inner); 
	return show(ret, &style_outer); 
}

string show_operator(char c)
{
	TRACE_FUNCTION(SHOW, show_operator(char));
	TRACE("(%s)", frmt("%c", c));
	Style style= S_STDERR | S_HAS_MARKER;
        string ret= show(string(1, c), &style);
	TRACE("ret= %s", ret);
	return ret;
}

string show_operator(string s)
{
	TRACE_FUNCTION(SHOW, show_operator(string));
	TRACE("(%s)", s);
	Style style= S_STDERR | S_HAS_MARKER;
        string ret= show(s, &style);
	TRACE("ret= %s", ret);
	return ret;
}

template <typename T>
string show_prefix(string prefix, const T &object, Style *style)
{
	TRACE_FUNCTION(SHOW, show_prefix);
	TRACE("%s", style_format(style));
	Style style_inner= Style::inner(style, S_HAS_MARKER);
	string s= show(object, &style_inner);
	string ret= prefix + s;
	Style style_outer= Style::outer(style, &style_inner, S_HAS_MARKER);
	ret= show(ret, &style_outer); 
	TRACE("ret= %s", ret);
	return ret;
}
