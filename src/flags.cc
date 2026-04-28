#include "flags.hh"

const char *flags_placed_phrases[C_ALL]= {
	"persistent", "optional", "trivial",
	nullptr, nullptr, nullptr,
	"newline-separated", "nul-separated", "stu-syntax",
	"no-dereference",
	nullptr, nullptr, nullptr, nullptr,
};

static_assert(sizeof(flags_placed_phrases) / sizeof(flags_placed_phrases[0]) == C_ALL,
	"Keep in sync with Flags");

Index flag_get_index(char c)
{
	TRACE_FUNCTION();
	TRACE("c= '%s'", frmt("%c", c));
	char *r= strchr((char *)flags_chars, c);
	assert(r);
	return r - flags_chars;
}

bool is_placed_flag_char(char c)
{
	char *r= strchr((char *)flags_chars, c);
	if (!r) return false;
	return (1 << (r - flags_chars)) & F_PLACED;
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
	for (Index i= 0; i < C_ALL; ++i) {
		Flags test= flags & (1u << i);
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
