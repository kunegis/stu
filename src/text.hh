#ifndef TEXT_HH
#define TEXT_HH

// TODO rename file to format.hh

/*
 * Various text manipulation functions.
 *
 * The formatting functions will become obsolete once we use C++20, around 2025.
 *
 * The reverse function will become obsolete once we switch to C++17, around 2022.
 */

#include <string.h>

string frmt(const char *format, ...)
/* Perform printf-like formatting.  Take the same arguments as printf() and
 * return a string.  Does *not* support passing string() objects.  */
#ifdef __GNUC__
	/* This declaration should be replaced by a sh/conf check */
	__attribute__ ((format(printf, 1, 2)))
#endif
;

void reverse_string(string &s);
/* Reverse the given string in-place */

/* fmt() allows *only* the unqualified '%s' format specifier with string
 * and const char * arguments, and '%%'.  Precisely, this allows any
 * argument that can be concatenated to a string with the '+' operator.  */

string fmt(const char *s);

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
