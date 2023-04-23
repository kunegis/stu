#include "format.hh"

bool src_need_quotes(const string &name)
{
	if (name.size() == 0)
		return true;

	if (name[0] == '-' || name[0] == '~' || name[0] == '+')
		return true;

	bool ret= false;
	for (char c:  name) {
		if (! isalnum(c) &&
		    ! strchr("+-./^`_~", c) &&
		    ! (c & 0x80))
			ret= true;
	}
	return ret;
}

string char_format(char c, Style style, bool &quotes)
{
	(void) style;

	if (c == ' ') {
		quotes= true;
		return " ";
	}
	else if (c == '\\')      return "\\\\";
	else if (c == '\0')      return "\\0";
	else if (c == '\n')	 return "\\n";
	else if (c == '\'') {
		if (quotes)
			return "\\'";
		else
			return "\'";
	}
	else if (c >= 0x20 && c <= 0x7E) {
		return string(1, c);
	}
	else {
		return frmt("\\%03o", (unsigned char) c);
	}
}

string char_format_err(char c)
{
	bool quotes= Color::quotes;
	string s= char_format(c, 0, quotes);

	if (quotes) {
		return fmt("%s'%s'%s", Color::word, s, Color::end);
	} else {
		return fmt("%s%s%s", Color::word, s, Color::end);
	}
}

string multichar_format_err(string s)
{
	bool quotes= Color::quotes;

	if (quotes) {
		return fmt("%s'%s'%s", Color::word, s, Color::end);
	} else {
		return fmt("%s%s%s", Color::word, s, Color::end);
	}
}

string name_format(string name, Style style, bool &quotes)
{
	if (name.empty()) {
		if (! (style & S_NOEMPTY))
			quotes= true;
		return "";
	}

	if (!quotes && strchr(name.c_str(), ' '))
		quotes= true;

	string ret(4 * name.size(), '\0');
	char *const q_begin= &ret[0], *q= q_begin;

	for (size_t i= 0;  i < name.size();  ++i) {
		char c= name[i];
		unsigned char cu= (unsigned char) c;
		if (c == ' ') {
			*q++= ' ';
		} else if (c == '\\') {
			*q++= '\\';
			*q++= '\\';
		} else if (c == '\0') {
			*q++= '\\';
			*q++= '0';
		} else if (c == '\n') {
			*q++= '\\';
			*q++= 'n';
		} else if (c == '\'') {
			if (quotes) {
				*q++= '\'';
				*q++= '\\';
				*q++= '\'';
				*q++= '\'';
			} else {
				*q++= '\'';
			}
		} else if (cu >= 0x21 && cu != 0xFF) {
			*q++= c;
		} else {
			*q++= '\\';
			*q++= '0' + (cu >> 6);
			*q++= '0' + ((cu >> 3) & 7);
			*q++= '0' + (cu & 7);
		}
	}
	ret.resize(q - q_begin);
	return ret;
}

string name_format_err(string name)
{
	bool quotes= Color::quotes;
	string s= name_format(name, 0, quotes);

	return fmt("%s%s%s%s%s",
		   Color::word,
		   quotes ? "'" : "",
		   s,
		   quotes ? "'" : "",
		   Color::end);
}

string name_format_src(string name)
{
	Style style= 0;
	bool quotes= src_need_quotes(name);
	string text= name_format(name, style, quotes);

	return fmt("%s%s%s",
		   quotes ? "'" : "",
		   text,
		   quotes ? "'" : "");
}

string dynamic_variable_format_err(string name)
{
	bool quotes= false;
	string s= name_format(name, S_MARKERS, quotes);
	return fmt("%s$[%s%s%s]%s",
		   Color::word,
		   quotes ? "'" : "",
		   s,
		   quotes ? "'" : "",
		   Color::end);
}

string prefix_format_err(string name, string prefix)
{
	assert(! prefix.empty());
	bool quotes= false;
	string s= name_format(name, S_MARKERS, quotes);
	return fmt("%s%s%s%s%s%s",
		   Color::word,
		   prefix,
		   quotes ? "'" : "",
		   s,
		   quotes ? "'" : "",
		   Color::end);
}
