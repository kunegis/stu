#ifndef FORMAT_HH
#define FORMAT_HH

/* 
 * Format-like functions: 
 *
 * - format(...) formats the content according to the exact
 *   specification, but never surrounds it by quotes or color
 * - format_word() returns a string suitable for inclusion in a message
 *   on STDERR, including quotes and color, as appropriate. 
 * - format_out() returns the same as format_word(), but for STDOUT. 
 * - format_src() formats an expression as if if was part of the source,
 *   e.g., use quotes only if the name contains characters that need to
 *   be quoted.  
 * - raw() does not escape anything. 
 *
 * Format functions are defined in the source files where their datatype
 * is defined. 
 */

#include "color.hh"

typedef unsigned Style;
enum 
{
	/* There will be some markers around or to the left of the text */
	S_MARKERS=       1 << 0,

	/* Don't need quote around empty content */ 
	S_NOEMPTY=       1 << 1,
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
	else {   
		return frmt("\\%03o", (unsigned char) c);
	}
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

string multichar_format_word(string s)
{
	bool quotes= Color::quotes;

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
			if (quotes) {
				*q++= '\'';
				*q++= '\\'; 
				*q++= '\'';
				*q++= '\'';
			} else {
				*q++= '\'';
			}
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
	string s= name_format(name, S_MARKERS, quotes); 
	return fmt("%s%s%s%s%s%s",
		   Color::word,
		   prefix,
		   quotes ? "'" : "",
		   s,
		   quotes ? "'" : "",
		   Color::end); 
}

/*
 * Whether a string needs to be quoted in the shell or in Stu (which
 * have the same quoting syntax.)  This is used so that Stu output looks
 * like input to the shell.  Note that for strings beginning with ~ or
 * -, quoting is not enough:  they have to be separated with '--'
 * additionally in the shell.   
 */
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

#endif /* ! FORMAT_HH */
