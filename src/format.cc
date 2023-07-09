#include "format.hh"

#include <stdarg.h>

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
		unreachable();
	}

	assert(n == (int) ret.size());

	return ret;
}

void reverse_string(string &s)
{
	const auto size= s.size();
	for (size_t i= 0; i < size / 2; ++i) {
		std::swap(*(s.begin() + i), *(s.begin() + size - 1 - i));
	}
}

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
			should_not_happen();
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
		should_not_happen();
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
		should_not_happen();
		return ret;
	}
	assert(*s == 's');
	++s;

	return ret + value + fmt(s, args...);
}
