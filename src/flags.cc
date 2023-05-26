#include "flags.hh"

const char *const flags_chars= "pot[@$n0<*%";
const char *flags_phrases[C_PLACED]= {"persistent", "optional", "trivial"};

unsigned flag_get_index(char c)
{
	switch (c) {
	case 'p':  return I_PERSISTENT;
	case 'o':  return I_OPTIONAL;
	case 't':  return I_TRIVIAL;
	case 'n':  return I_NEWLINE_SEPARATED;
	case '0':  return I_NUL_SEPARATED;
	default:   assert(false);  return 0;
	}
}

string show(Flags flags, Style *style)
{
	string ret;
	for (unsigned i= 0;  i < C_ALL;  ++i)
		if (flags & (1 << i)) {
			ret += flags_chars[i];
		}
	if (ret.empty())
		return ret;
	Style style_inner= Style::inner(style, false, S_HAS_MARKER);
	ret= '-' + show(ret, &style_inner);
	Style style_outer= Style::outer(style, &style_inner);
	return show(ret, &style_outer);
}

string done_format(Done done)
{
	char ret[7]= "[0000]";
	for (int i= 0;  i < 4;  ++i)
		ret[1+i] |= 1 & done << i;
	return ret;
}

Done done_from_flags(Flags flags)
{
	return (~flags & (F_PERSISTENT | F_OPTIONAL)) * (1 | (~flags & F_TRIVIAL));
}
