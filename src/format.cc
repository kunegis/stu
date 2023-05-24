#include "format.hh"

Style::Style(Show_Bits bits_)
	:  bits(bits_)
{
	init(); 
	assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1);
}

Style::Style(Style *style, bool may_show_quotes)
{
	bits= S_DONT_SHOW_COLOR;
	if (style) {
		bits |= style->bits;
	} else {
		bits |= S_DEFAULT;
		init(); 
	}
	if (! may_show_quotes)
		bits |= S_DONT_SHOW_QUOTES;
	assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1);
}

Style::Style(const Style *style, const Style *style_inner, int)
{
	bits= S_DONT_FORMAT;
	if (style) {
		bits |= style->bits;
	} else {
		bits |= S_DEFAULT;
		init(); 
	}
	if (style_inner && style_inner->bits & S_DONT_SHOW_QUOTES) {
		bits |= *style_inner & S_NEED_QUOTES;
	} else {
		bits |= S_DONT_SHOW_QUOTES;
	}

	// TODO rm the whole if
	if (!(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1)) {
		printf("x\n"); 
		_Exit(99); 
	}
	
	assert(((bits & S_STDOUT)!=0) + ((bits & S_STDERR)!=0) == 1);
}

void Style::init()
{
	if (bits & S_STDOUT) {
		if (Color::out_quotes)
			bits |= S_NEED_QUOTES;
	} else if (bits & S_STDERR) {
		if (Color::err_quotes)
			bits |= S_NEED_QUOTES;
	} else
		assert(false);
}

string show(string name, Style *style)
{
	assert(!style || ((*style & S_STDOUT)!=0) + ((*style & S_STDERR)!=0) == 1); 

	Style a_style= S_DEFAULT;
	if (!style)
		style= & a_style; 
	
	if (name.empty() && ! (*style & S_NO_EMPTY)) {
		style->set();
	}

	for (char c:  name) 
		if (! isalnum(c) &&
		    ! strchr("+-./^`_~", c) &&
		    ! (c & 0x80)) {
			style->set();
			break;
		}

	string ret(4 * name.size(), '\0');
	char *const p_begin= &ret[0], *p= p_begin;
	bool escape= false;
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
			*p++= '\\';
			*p++= '0' + (cu >> 6);
			*p++= '0' + ((cu >> 3) & 7);
			*p++= '0' + (cu & 7);
		}
		if (escape_this)
			escape= true;
	}
        ret.resize(p - p_begin);

	if (*style & S_DONT_SHOW_QUOTES)
		return ret;
	bool quotes= escape ? true : style->is();

//	if (ret.empty())
//		quotes= true;
	
	if (quotes) 
		ret= fmt("\"%s\"", ret);
	if (*style & S_STDOUT) {
		return fmt("%s%s%s", Color::stdout_highlight_on,
			   ret, Color::stdout_highlight_off); 
	} 
	if (*style & S_STDERR) {
		return fmt("%s%s%s", Color::stderr_highlight_on,
			   ret, Color::stderr_highlight_off);
	}
	assert(false); 
}

//string name_format_err(string name)
//{
//	bool quotes;
//	return name_format(name, S_ERR, quotes);
	//	Style style= S_WANT_ESCAPE | S_QUOTES * Color::quotes;
//	bool quotes= Color::quotes;
//	string s= name_format(name, style);

//	return fmt("%s%s%s%s%s",
//		   Color::word,
//		   style & S_QUOTES ? "'" : "",
//		   s,
//		   style & S_QUOTES ? "'" : "",
//		   Color::end);
//}

//string name_format_src(string name, Style style)
//{
//	bool quotes;
//	return name_format(name, style | S_SRC, quotes);
//	bool quotes= src_need_quotes(name, style);
//	style |= S_WANT_ESCAPE;
//	string text= name_format(name, style);
//	return fmt("%s%s%s",
//		   style & S_QUOTES ? "'" : "",
//		   text,
//		   style & S_QUOTES ? "'" : "");
//}

string show_dynamic_variable(string name, Style *style)
{
	Style style_inner(style, true);
	string s= show(name, &style_inner);
	string ret= fmt("$[%s]", s);
	Style style_outer(style, &style_inner, 0); 
	return show(ret, &style_outer); 
}

//bool src_need_quotes(const string &name, Style style)
//{
//	if (name.size() == 0)
//		return !(style & S_NOEMPTY);
//
//	if (name[0] == '-' || name[0] == '~' || name[0] == '+')
//		return true;
//
//	bool ret= false;
//	for (char c:  name) {
//		if (! isalnum(c) &&
//		    ! strchr("+-./^`_~", c) &&
//		    ! (c & 0x80)) {
//			ret= true;
//			break;
//		}
//	}
//	return ret;
//}

// string quote(string text, Style style, const Quotes *q)
// {
// 	if (style & S_INNER)
// 		return text;
// 	bool quotes;
// 	if (q) {
// 		quotes= q->is();
// 	} else {
// 		Quotes q_this(style);
// 		quotes= q_this.is(); 
// 	}

// 	if (text.empty())
// 		quotes= true;
	
// 	if (quotes) {
// 		text= fmt("\"%s\"", text);
// 	}
// 	if (style & S_OUT) {
// 		return fmt("%s%s%s", Color::stdout_highlight_on,
// 			   text, Color::stdout_highlight_off); 
// 	}
// 	if (style & S_ERR) {
// 		return fmt("%s%s%s", Color::stderr_highlight_on,
// 			   text, Color::stderr_highlight_off);
// 	}
// 	if (style & S_SRC) {
// 		return text;
// 	} 
// 	assert(false); 
// }

