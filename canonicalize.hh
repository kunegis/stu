#ifndef CANONICALIZE_HH
#define CANONICALIZE_HH

/* 
 * Canonicalization is the mapping of filenames and transient names to
 * unique names in their simplest form with respect to the filename
 * components '/' and '.'.
 * 
 * This function canonicalizes a string, and is used by higher-level
 * code to canonicalize actual names.  The function is called both on
 * entire names (when they are non-parametrized), as well as on parts of
 * names (when they are parametrized). 
 *
 * METHOD
 *
 *  - Fold /
 *      - Multiple / -> single /
 *	    - except for double slash at the start not followed by /
 *            (This is because POSIX mandates that a name starting with
 *            exactly two slashes is special.)
 *      - Remove ending /
 * 	    - except when the name contains only '/' characters, i.e.,
 *            when the name is '/' or or double slash. 
 *  - Fold .
 *      - ^/.$ -> /
 *      - ^./$ -> .
 *      - ^./ -> ''  [multiple times] [not when followed by parameter]
 *      - /./ -> /   [multiple times]
 *      - /.$ -> ''  [multiple times]
 *
 * Symlinks and '..' are not canonicalized by Stu.  As a general rule,
 * no stat(2) is performed to check whether name components exist.
 *
 * For further rules about canonicalization (which are outside the scope of this function), see the
 * manpage.  Some of the special rules are handled in this file. 
 */

typedef unsigned Canon_Flags;
/* Declared as integer so arithmetic can be performed on it */

enum
/* Each flags means:  The begin/end of the string is adjacent to the
 * very beginning/end of the name, rather than to a parameter.  */
{
	A_BEGIN 	= 1 << 0,
	A_END 		= 1 << 1,
};


char *canonicalize_string(Canon_Flags canon_flags, char *p);
/* Canonicalize the string starting at P in-place.
 * Return the end (\0) of the new string.  The operation never increases
 * the size of the string.  P is \0-terminated, on input and output. */ 

char *canonicalize_string(Canon_Flags canon_flags, char *p) 
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
	if (s - p == 2 && canon_flags & A_BEGIN) {
		*d++= '/';
		*d++= '/';
	} else if (s != p) {
		*d++= '/';
	}
	
	if (canon_flags & A_BEGIN) 
		d_head= d; 
	
	/* Main loop:  collapse multiple slashes */

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
	if (canon_flags & A_END) 
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

	if (canon_flags & A_END && s[0] == '/' && s[1] == '.' && s[2] == '\0') {
		if (canon_flags & A_BEGIN)
			*++d= '\0'; 
		else {
			*d= '\0'; 
		}
		s += 2; 
	} else if (canon_flags & A_BEGIN && canon_flags & A_END
		   && s[0] == '.' && s[1] == '/' && s[2] == '\0') {
		*++d= '\0'; 
		s += 2; 
	} else if (canon_flags & A_BEGIN && ! (canon_flags & A_END)
		   && s[0] == '.' && s[1] == '/' && s[2] == '\0') {
		/* Keep a lone './' in the first component followed
		 * by a parameter.  The meaning is that the following
		 * parameter can only be matched by a value not
		 * beginning by a slash.  */
		s += 2;
		d += 2;
	} else {
		while (canon_flags & A_BEGIN && s[0] == '.' && s[1] == '/') {
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
	 * In general, what is done here is wrong because XXX/.. is not
	 * equivalent to '.' when XXX is a symlink.  Thus, a system call
	 * would be needed to resolve such cases, and systems calls are
	 * not used in Stu for canonicalization. 
	 */
	/* Note: This code does not take into account CANON_FLAGS -- it
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

#endif /* ! CANONICALIZE_HH */
