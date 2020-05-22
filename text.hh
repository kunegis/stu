#ifndef TEXT_HH
#define TEXT_HH

/* 
 * Text formatting functions. 
 *
 * These will become obsolete once we use C++20, around 2025. 
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
/* Call snprintf() twice: once to compute the needed size, then once to
 * actually print everything.  */ 
{
	va_list ap;
	char buf[2];
	int n;

	va_start(ap, format); 
	n= vsnprintf(buf, 2, format, ap);
	va_end(ap); 

	string ret(n, '\0');

	va_start(ap, format); 
	n= vsnprintf(&ret[0], n+1, format, ap); 
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
			/* Missing argument, or one too many %s */ 
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

#endif /* ! TEXT_HH */
