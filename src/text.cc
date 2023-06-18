#include "text.hh"

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
		assert(false);
		perror("snprintf");
		abort();
	}

	assert(n == (int) ret.size());

	return ret;
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
			assert(false);
			break;
		}
		ret += '%';
		++s;
	}
	return ret;
}

void reverse_string(string &s)
{
	const auto size= s.size();
	for (size_t i= 0; i < size / 2; ++i) {
		swap(*(s.begin() + i), *(s.begin() + size - 1 - i));
	}
}
