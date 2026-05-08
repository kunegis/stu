#include "show_flags.hh"

void render(Flag_View fv, Parts &parts, Rendering rendering)
{
	parts.append_marker("-");
	render(string(1, fv.c), parts, rendering);
}

#ifndef NDEBUG

void render(Flags_View flags_view, Parts &parts, Rendering rendering)
{
	TRACE_FUNCTION();
	if (!(rendering & R_SHOW_FLAGS))
		return;
	string ret;
	for (Index i= 0; i < C_ALL; ++i) {
		Flags test= flags_view.flags & (1u << i);
		if (test) {
			ret += flags_chars[i];
		}
	}
	if (ret.empty())
		return;
	ret= '-' + ret;
	parts.append_operator(ret);
}

#endif /* ! NDEBUG */
