#ifndef FORMAT_HH
#define FORMAT_HH

/*
 * These functions will become obsolete once we use C++20.
 *
 * The reverse function will become obsolete once we switch to C++17.
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

string fmt(const char *s);
template<typename T, typename... Args> string fmt(const char *s, T value, Args... args);
/* fmt() allows only the unqualified '%s' format specifier with string and const char *
 * arguments, and '%%'.  Precisely, this allows any argument that can be concatenated to a
 * string with the '+' operator. */

#endif /* ! FORMAT_HH */
