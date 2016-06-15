#ifndef FORMAT_HH
#define FORMAT_HH

#include "color.hh"

typedef unsigned Style;
enum 
{
	/* Set individually */
	S_LEFT_MARKER=   1 << 0,
	S_RIGHT_MARKER=  1 << 1,
	S_MARKERS=       S_LEFT_MARKER | S_RIGHT_MARKER,

	/* Don't need quote around empty content */ 
	S_NOEMPTY=       1 << 2,
};

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
	else    
		return frmt("\\%03o", (unsigned char) c);
}

string char_format_word(char c) 
{
	bool quotes= Color::quotes;
	string s= char_format(c, 0, quotes); 

	if (quotes) {
		return fmt("%s'%s'%s",
			   Color::word, s, Color::end); 
	} else {
		return fmt("%s%s%s", 
			   Color::word, s, Color::end); 
	}
}

string name_format(string name, Style style, bool &quotes) 
{
	if (name == "") {
		if (! (style & S_NOEMPTY))  quotes= true; 
		return ""; 
	}

	if (! quotes && strchr(name.c_str(), ' '))
		quotes= true;

	string ret(4 * name.size(), '\0');
	char *const q_begin= (char *) ret.c_str(), *q= q_begin; 
	for (const char *p= name.c_str();  *p;  ++p) {
		char c= *p;
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
			if (quotes)
				*q++= '\\'; 
			*q++= '\''; 
		} else if (cu >= 0x21 && cu != 0xFF) {
			*q++= *p;
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

string name_format_word(string name)
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

string dynamic_variable_format_word(string name)
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

string prefix_format_word(string name, string prefix)
{
	assert(prefix != ""); 
	bool quotes= false;
	string s= name_format(name, S_LEFT_MARKER, quotes); 
	return fmt("%s%s%s%s%s%s",
		   Color::word,
		   prefix,
		   quotes ? "'" : "",
		   s,
		   quotes ? "'" : "",
		   Color::end); 
}

#endif /* ! FORMAT_HH */
