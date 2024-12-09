#ifndef CANONICALIZE_HH
#define CANONICALIZE_HH

/*
 * Canonicalization is the mapping of filenames and transient names to unique names in
 * their simplest form with respect to the filename components '/' and '.'.
 *
 * This function canonicalizes a string, and is used by higher-level code to canonicalize
 * actual names.  The function is called both on entire names (when they are
 * non-parametrized), as well as on parts of names (when they are parametrized).
 *
 * METHOD
 *
 *  - Fold /
 *      - Multiple / -> single /
 *          - except for double slash at the start not followed by /
 *            (This is because POSIX mandates that a name starting with
 *            exactly two slashes is special.)
 *      - Remove ending /
 *          - except when the name contains only '/' characters, i.e.,
 *            when the name is '/' or double slash.
 *  - Fold .
 *      - ^/.$ -> /
 *      - ^./$ -> .
 *      - ^./ -> ''  [multiple times] [not when followed by parameter]
 *      - /./ -> /   [multiple times]
 *      - /.$ -> ''  [multiple times]
 *
 * Symlinks and '..' are not canonicalized by Stu.  As a general rule, no stat(2) is
 * performed to check whether name components exist.
 *
 * For further rules about canonicalization see the manpage.  Some of the special rules
 * are handled in this file.
 */

typedef unsigned Canonicalize_Flags;
/* Declared as integer so arithmetic can be performed on it */

enum
/* Each flags means:  The begin/end of the string is adjacent to beginning/end
 * of the name, rather than to a parameter. */
{
	A_BEGIN  = 1 << 0,
	A_END    = 1 << 1,
};

char *canonicalize_string(Canonicalize_Flags canonicalize_flags, char *p);
/* Canonicalize the string starting at P in-place.  Return the end (\0) of the new string.
 * The operation never increases the size of the string.  P is \0-terminated, on input and
 * output. */

#endif /* ! CANONICALIZE_HH */
