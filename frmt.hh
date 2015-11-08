#ifndef FRMT_HH
#define FRMT_HH

#include <stdarg.h>
#include <stdlib.h>

/* Two formatting functions.
 */

/* This is a convenient function for performing printf-like formatting
 * in C++.  The function (named frmt()) takes the same arguments as
 * printf() and returns a C++ string.  Note:  does *not* support passing
 * string() objects. 
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

	if (n < 0) { /* encoding error */ 
		assert(false); 
		perror("snprintf");
		abort(); 
	}

	assert(n == (int) ret.size()); 

	return ret; 
}

/* fmt() allows *only* string or char-pointer objects.  Technically,
 * this allows any argument that can be concatenated to a string with
 * the '+' operator.  Allows only the unqualified %s format.  
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
				/* Too many arguments */ 
				assert(false);
				return string(q);
		}
		assert(*q == '%'); 
		string ret(s, q - s); 
		s = q + 1;
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

#endif /* ! FRMT_HH */
