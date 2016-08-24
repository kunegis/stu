#ifndef TEXT_HH
#define TEXT_HH

/* 
 * Text functions. 
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <string>

/* 
 * This is a convenient function for performing printf-like formatting
 * in C++.  The function takes the same arguments as printf() and
 * returns a C++ string.  Note:  does *not* support passing string()
 * objects.  
 */

/* This declaration should be replaced by an Autoconf macro. */
#ifdef __GNUC__
string frmt(const char *format, ...) 
	__attribute__ ((format(printf, 1, 2)));
#endif

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
 * argument that can be concatenated to a string with the '+' operator.  */

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
