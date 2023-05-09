#include "format.hh"

string char_format(char c, Style style, Quotes *q)
{
	string ret;
	
	if (c == ' ') {
		if (q)  q->set(); 
		ret= " ";
	}
	else if (c == '\\') {
		if (q && q->is())
			ret= "\\\\";
		else
			ret= "\\";
	}
	else if (c == '\0')        ret= "\\0";
	else if (c == '\n')	   ret= "\\n";
	else if (c == '\'') {
		if (q && q->is())
			return "\\'";
		else
			return "\'";
	} else if (c >= 0x20 && c <= 0x7E) {
		ret= string(1, c);
	} else {
		ret= frmt("\\%03o", (unsigned char) c);
	}
	return quote(ret, style, q); 
}

// string char_format_err(char c)
// {
// 	bool quotes= Color::quotes;
// 	string s= char_format(c, 0, quotes);

// 	if (quotes) {
// 		return fmt("%s'%s'%s", Color::word, s, Color::end);
// 	} else {
// 		return fmt("%s%s%s", Color::word, s, Color::end);
// 	}
// }

// string multichar_format_err(string s)
// {
// 	bool quotes= Color::quotes;

// 	if (quotes) {
// 		return fmt("%s'%s'%s", Color::word, s, Color::end);
// 	} else {
// 		return fmt("%s%s%s", Color::word, s, Color::end);
// 	}
// }

string name_format(string name, Style style, Quotes *q)
{
//	assert((style & S_WANT_ESCAPE) || !(style & S_QUOTES_SINGLE));
	assert((style & S_OUT)!=0 + (style & S_ERR)!=0 + (style & S_SRC)!=0 == 1); 

	if (name.empty() && ! (style & S_NO_EMPTY)) {
		if (q)  q->set();
	}

	for (char c:  name) 
		if (! isalnum(c) &&
		    ! strchr("+-./^`_~", c) &&
		    ! (c & 0x80)) {
			if (q)  q->set();
			break;
		}
//	if (!quotes && strchr(name.c_str(), ' '))
//		quotes= true;

	string ret(4 * name.size(), '\0');
	char *const p_begin= &ret[0], *p= p_begin;

	for (size_t i= 0;  i < name.size();  ++i) {
		char c= name[i];
		unsigned char cu= (unsigned char) c;
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
			if (q && q->is()) {
				*p++= '\'';
				*p++= '\\';
				*p++= '\'';
				*p++= '\'';
			} else {
				*p++= '\'';
			}
		} else if (cu >= 0x21 && cu != 0xFF) {
			*p++= c;
		} else {
			*p++= '\\';
			*p++= '0' + (cu >> 6);
			*p++= '0' + ((cu >> 3) & 7);
			*p++= '0' + (cu & 7);
		}
	}
        ret.resize(p - p_begin);
	return quote(ret, style, q);
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

string dynamic_variable_format(string name, Style style, Quotes *q)
{
	string s= name_format(name, style | S_INNER, q);
	string ret= fmt("$[%s]", s);
	return quote(ret, style, q); 
}

string prefix_format(string name, string prefix, Style style, Quotes *q)
{
	assert(! prefix.empty());
//	Style style= S_MARKERS | S_WANT_ESCAPE;
	string s= name_format(name, style | S_INNER, q);
	return quote(prefix + s, style, q); 
	
//	return fmt("%s%s%s%s%s%s",
//		   Color::word,
//		   prefix,
//		   style & S_QUOTES ? "'" : "",
//		   s,
//		   style & S_QUOTES ? "'" : "",
//		   Color::end);
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

string quote(string text, Style style, const Quotes *q)
{
	if (style & S_INNER)
		return text;
	if (q && q->is()) {
		text= fmt("\"%s\"", text);
	}
	if (style & S_OUT) {
		return fmt("%s%s%s", Color::stdout_highlight_on,
			   text, Color::stdout_highlight_off); 
	}
	if (style & S_ERR) {
		return fmt("%s%s%s", Color::stderr_highlight_on,
			   text, Color::stderr_highlight_off);
	}
	if (style & S_SRC) {
		return text;
	} 
	assert(false); 
}

