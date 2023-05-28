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
	}

	for (size_t i= 0;  !style->is() && i < name.size();  ++i) {
		char c= name[i];
		if (c >= 0 && c <= ' ' || c == 0x7F) {
			style->set();
		}
	}

	string ret(4 * name.size(), '\0');
	char *const p_begin= &ret[0], *p= p_begin;
	if (*style & S_OUTER)
		ret= name;
	else {
		// TODO refactor to function 
		for (size_t i= 0;  i < name.size();  ++i) {
			char c= name[i];
			unsigned char cu= (unsigned char) c;
			bool escape_this= true;
			if (c == ' ') {
				*p++= ' ';
			} else if (c == '\\') {
				*p++= '\\';
				*p++= '\\';
			} else if (c == '\0') {
				*p++= '\\';
				*p++= '0';
			} else if (c == '\n') {
				*p++= '\\';
				*p++= 'n';
			} else if (c == '\'') {
				if (style->is()) {
					*p++= '\'';
					*p++= '\\';
					*p++= '\'';
					*p++= '\'';
				} else {
					*p++= '\'';
				}
			} else if (cu >= 0x21 && cu != 0xFF) {
				*p++= c;
				escape_this= false;
			} else {
				// TODO don't use octal.  Instead, use something
				// that is also valid Stu input in double quotes.
				*p++= '\\';
				*p++= '0' + (cu >> 6);
				*p++= '0' + ((cu >> 3) & 7);
				*p++= '0' + (cu & 7);
			}
			if (escape_this)
				style->set();
		}
		ret.resize(p - p_begin);
	}

	bool quotes= style->is() ||
		((*style & S_NEED_QUOTES_NOCOLOR) && !(*style & S_HAS_MARKER) && !(*style & S_OUTER));
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
