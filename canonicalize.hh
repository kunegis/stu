#ifndef CANONICALIZE_HH
#define CANONICALIZE_HH

/* 
 * Canonicalization refers to the mapping of filenames and transient
 * names to unique names in which the elements '/', '.' and '..' are
 * reduced to their simplest form. 
 *
 * ALGORITHM
 *
 *  - Fold /
 *      - Multiple / -> single /, except for double slash at the start
 *        not followed by /  
 *      - Remove ending /, except when the name contains only '/'
 *        characters 
 *  - Fold .
 *      - ^/.$ -> /
 *      - ^./$ -> .
 *      - ^./ -> ''  [multiple times] [not when followed by parameter]
 *      - /./ -> /   [multiple times]
 *      - /.$ -> ''  [multiple times]
 *  - Fold .. 
 *      - ^/.. -> '' or '/' when otherwise empty [multiple times]
 *      - [^/]+/.. -> '' or '.' when otherwise empty [multiple times]
 *
 * Symlinks are not handled by Stu.  Also, no stat(2) is performed, to
 * check whether name components exist.   
 */

char *canonicalize_string(char *dst, const char *src);
/* Canonicalize the string starting at SRC, writing output to DEST.
 * Return the end (\0) of the new string.  The operation never increases
 * the size of the string.  SRC and DEST may be equal.  SRC is
 * \0-terminated, and a terminating \0 is written to DST.  */

char *canonicalize_string(char *const dest, const char *const src) 
/* Three passes are made, for '/', '.' and '.., in that order. 
 * In the first pass, we copy SRC to DEST.  The two subsequent passes
 * operate within DEST.  */
// TODO change to always work in-place, if not needed otherwise
{
	/*
	 * Fold '/'
	 */

	char *d= dest; 

#ifndef NDEBUG
	const char *const limit= dest + strlen(src); 
#endif

	/* Slashes at the start */
	const char *s= src;
	while (*s == '/') 
		++s;
	if (s - src == 2) {
		*d++= '/';
		*d++= '/';
	} else if (s != src) {
		*d++= '/';
	}

	const char *d_head= d; /* After the leading slashes */
	
	/* Main loop: 
	 * collapse multiple slashes */
	bool last_slash= false;
	while (*s) { // TODO use strchr here
		if (*s == '/' && last_slash) {
			++s;
			continue;
		}
		last_slash= *s == '/';
		*d++= *s++; 
	}

	/* Remove trailing slash */
	while (d > d_head && d[-1] == '/') 
		--d;

#ifndef NDEBUG
	assert(dest <= limit); 
#endif

	*d= '\0'; 

	/*
	 * Fold '.'
	 */

	s= dest;
	d= dest; 

	if (s[0] == '/' && s[1] == '.' && s[2] == '\0') {
		*++d= '\0'; 
	} else if (s[0] == '.' && s[1] == '/' && s[2] == '\0') {
		*++d= '\0'; 
	} else {
		while (s[0] == '.' && s[1] == '/') {
			s += 2; 
		}
		const char *m;
		while ((m= strstr(s, "/./"))) {
			memmove(d, s, m - s);
			d += m - s;
			*d++ = '/'; 
			s += m - s + 3; 
		}
		size_t l= strlen(s);
		memmove(d, s, l + 1); 
		s += l;
		d += l;
		while (d - 2 >= dest && d[-2] == '/' && d[-1] == '.') {
			d -= 2;
			*d= '\0';
		}
	}

	/*
	 * Fold '..'
	 */

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
			if (*s == '/')
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

	assert(*s == '\0'); 
	assert(*d == '\0'); 

	return d; 
}

#endif /* ! CANONICALIZE_HH */
