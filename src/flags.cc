#include "flags.hh"

const char *flags_phrases[C_PLACED]= {"persistent", "optional", "trivial"};
static_assert(sizeof(flags_phrases) / sizeof(flags_phrases[0]) == C_PLACED,
	      "Keep in sync with Flags");

unsigned flag_get_index(char c)
{
	char *r= strchr((char *)flags_chars, c);
	assert(r);
	return r - flags_chars;
}

void render(Flag_View fv, Parts &parts, Rendering rendering)
{
	parts.append_marker("-");
	render(string(1, fv.c), parts, rendering);
}

#ifndef NDEBUG

bool render_flags(Flags flags, Parts &parts, Rendering rendering)
{
	TRACE_FUNCTION();
	if (!(rendering & R_SHOW_FLAGS))
		return false;
	string ret;
	for (unsigned i= 0; i < C_ALL; ++i) {
		unsigned test= flags & (1u << i);
		if (test) {
			ret += flags_chars[i];
		}
	}
	if (ret.empty())
		return false;
	ret= '-' + ret;
	parts.append_operator(ret);
	return true;
}

string show_flags(Flags flags, Style style)
{
	Parts parts;
	render_flags(flags, parts, R_SHOW_FLAGS);
	return show(parts, style);
}

#endif /* ! NDEBUG */
