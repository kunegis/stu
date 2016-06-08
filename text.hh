#ifndef TEXT_HH
#define TEXT_HH

/* Text functions. 
 */

#include <stdarg.h>
#include <string.h>

#include <string>

#include "color.hh"

/* Is the character a space in the C locale?  Note:  we don't use
 * isspace() because isspace() uses the current locale and may consider
 * locale-specific characters which we don't want to cover in this
 * function, whereas the syntax of Stu specifies that only these six
 * characters count as whitespace.  */
bool is_space(char c) 
{
	return c != '\0' && nullptr != strchr(" \n\t\v\r\f", c);
}

/* Format functions: 
 *
 * - format_err() returns the same as format_out(), with color
 *   codes.  Used only on standard error output. 
 * - format_out() returns an optionally quoted and printable representation
 *   of the target, without color codes. 
 * - format_mid() is used when brackets of any form are added around by
 *   the caller.  Uses quotes only when weird characters are contained. 
 * - format_semi() is used when an operator is present on only one side
 *   of a string. 
 * - format_raw() does not escape anything
 *
 * Format functions are defined in the source files where their datatype
 * is defined. 
 */

/* This is a convenient function for performing printf-like formatting
 * in C++.  The function takes the same arguments as printf() and
 * returns a C++ string.  Note:  does *not* support passing string()
 * objects.  
 */
string frmt(const char *format, ...) 
	__attribute__ ((format(printf, 1, 2)));
string frmt(const char *format, ...) 
{
	va_list ap;

	/* Call snprintf() twice:  once to compute the needed size, then
	 * once to actually print everything */ 

	char buf[2];
	va_start(ap, format); 
	int n= vsnprintf(buf, 2, format, ap);
	va_end(ap); 

	string ret(n, '\0');

	va_start(ap, format); 
	n= vsnprintf((char *) ret.c_str(), n+1, format, ap); 
	va_end(ap); 

	if (n < 0) { 
		/* Encoding error in the format string */ 
		assert(false); 
		perror("snprintf");
		abort(); 
	}

	assert(n == (int) ret.size()); 

	return ret; 
}

/* fmt() allows *only* the unqualified '%s' format specifier with string
 * and const char * arguments, and '%%'.  Precisely, this allows any
 * argument that can be concatenated to a string with the '+' operator.  
 */

/* Without args */ 
string fmt(const char *s)
{
	string ret; 
	while (*s) {
		const char *q= strchr(s, '%');
		if (!q) {
			ret += s;
			break;
		}
		assert(*q == '%'); 
		ret += string(s, q - s); 
		s= q + 1;
		if (*s != '%') {
			/* Missing argument */ 
			assert(false);
			break;
		}
		ret += '%'; 
		++s;
	}
	return ret; 
}

template<typename T, typename... Args>
string fmt(const char *s, T value, Args... args)
{
	const char *q= strchr(s, '%');
	if (!q) { 
		/* Too many arguments; not enough '%s' */ 
		assert(false);
		return string(q);
	}
	assert(*q == '%'); 
	string ret(s, q - s); 
	s= q + 1;
	if (*s == '%') {
		ret += '%';
		++s;
		return ret + fmt(s, value, args...); 
	}
	if (*s != 's') {
		/* Invalid format specifier */ 
		assert(false);
		return ret; 
	}
	assert(*s == 's');
	++s;

	return ret + value + fmt(s, args...); 
}

/* Format a character for output. */ 
string char_format_mid(char c)
{
	assert(0x5C == '\\'); 

	string text_char;
	if (c >= 0x20 && c <= 0x7E && c != 0x5C) 
		text_char= c;
	else if (c == 0x5C)
		text_char= "\\\\";
	else if (c == 0x00)
		text_char= "\\0";
	else
		text_char= frmt("\\%03o", (unsigned char) c);

	return text_char; 
}

string char_format_err(char c) 
{
	return fmt("%s%s%s", Color::beg_name_quoted, char_format_mid(c), Color::end_name_quoted); 
}

/* Padding for verbose output (option -v).  During the lifetime of an
 * object, padding is increased by one.  */
class Verbose
{
private:
	static string padding_current;

public:
	Verbose() 
	{
		padding_current += "   ";
	}

	~Verbose() 
	{
		padding_current.resize(padding_current.size() - 3);
	}

	static const char *padding() {
		return padding_current.c_str(); 
	}
};

string Verbose::padding_current= "";

#endif /* ! TEXT_HH */
