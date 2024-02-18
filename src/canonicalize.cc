#include "canonicalize.hh"

#include <string.h>

char *canonicalize_string(Canonicalize_Flags canonicalize_flags, char *p)
/* Two passes are made, for '/' and '.', in that order. */
{
	/*
	 * Fold '/'
	 */

	char *d= p;

	/* After the leading slashes when A_BEGIN, otherwise D */
	const char *d_head= d;

#ifndef NDEBUG
	const char *const limit= d + strlen(d);
#endif

	/* Slashes at the start */
	const char *s= p;
	while (*s == '/')
		++s;
	if (s - p == 2 && canonicalize_flags & A_BEGIN) {
		*d++= '/';
		*d++= '/';
	} else if (s != p) {
		*d++= '/';
	}

	if (canonicalize_flags & A_BEGIN)
		d_head= d;

	/* Collapse multiple slashes */
	while (*s) {
		const char *s_next= strchr(s, '/');
		if (! s_next) {
			s_next= strchr(s, '\0');
			memmove(d, s, s_next - s);
			d += s_next - s;
			s= s_next;
			break;
		}
		assert(*s_next == '/');
		memmove(d, s, s_next - s);
		d += s_next - s;
		s= s_next;
		while (*s == '/')
			++s;
		*d++= '/';
	}

	/* Remove trailing slashes */
	if (canonicalize_flags & A_END)
		while (d > d_head && d[-1] == '/')
			--d;

#ifndef NDEBUG
	assert(p <= limit);
#endif

	*d= '\0';

	/*
	 * Fold '.'
	 */

	s= p;
	d= p;

	if (canonicalize_flags & A_END && s[0] == '/' && s[1] == '.' && s[2] == '\0') {
		if (canonicalize_flags & A_BEGIN) {
			*++d= '\0';
		} else {
			*d= '\0';
		}
		s += 2;
	} else if (canonicalize_flags & A_BEGIN && canonicalize_flags & A_END
		   && s[0] == '.' && s[1] == '/' && s[2] == '\0') {
		*++d= '\0';
		s += 2;
	} else if (canonicalize_flags & A_BEGIN && ! (canonicalize_flags & A_END)
		   && s[0] == '.' && s[1] == '/' && s[2] == '\0') {
		/* Keep a lone './' in the first component followed by a parameter.  The
		 * meaning is that the following parameter can only be matched by a value
		 * not beginning by a slash. */
		s += 2;
		d += 2;
	} else {
		while (canonicalize_flags & A_BEGIN && s[0] == '.' && s[1] == '/') {
			s += 2;
		}
		const char *m;
		while ((m= strstr(s, "/./"))) {
			memmove(d, s, m - s + 1);
			d += m - s + 1;
			s= m + 3;
			while (s[0] == '.' && s[1] == '/')
				s += 2;
		}
		size_t l= strlen(s);
		memmove(d, s, l + 1);
		s += l;
		d += l;
		while (d - 2 >= p && d[-2] == '/' && d[-1] == '.') {
			d -= 2;
			*d= '\0';
		}
	}

#if 0
	/*
	 * Fold '..' -- The code is kept here but not used.
	 * In general, what is done here is wrong because ABC/.. is not
	 * equivalent to '.' when ABC is a symlink.  Thus, a system call
	 * would be needed to resolve such cases, and systems calls are
	 * not used in Stu for canonicalization.
	 */
	/* Note: This code does not take into account CANONICALIZE_FLAGS -- it
	 * behaves as if both were set */

	s= dest;
	d= dest;

	if (s[0] == '/' && s[1] == '/' && s[2] == '.' && s[3] == '.'
	    && (s[4] == '/' || s[4] == '\0')) {
		d += 2;
		s += 4;
	}

	const char *m;
	while ((m= strstr(s, "/.."))) {
		if (m[3] == '/' || m[3] == '\0') {
			memmove(d, s, m - s);
			d += m - s;
			if (d == dest) {
				*d++= '/';
			} else {
				assert(d > dest && d[-1] != '/');
				do
					--d;
				while (d > dest && d[-1] != '/');
			}
			s= m + 3;
			if (*s == '/' && d == dest)
				++s;
		} else {
			memmove(d, s, m + 3 - s);
			d += m + 3 - s;
			s= m + 3;
		}
	}
	size_t l= strlen(s);
	memmove(d, s, l + 1);
	s += l;
	d += l;
	if (d == dest) {
		*d++= '.';
		*d= '\0';
		assert(d <= s);
	}
#endif /* 0 */

	assert(*s == '\0');
	assert(*d == '\0');
	return d;
}
