#include "show_flags.hh"

Flag_View::Flag_View(const Placed_Flags &placed_flags, Index index_)
{
	assert(index_ < C_ALL);
	index= index_;
	const Place &place= placed_flags.place_by_index(index);
	long_flag= place.bits & Place::Bits::LONG_FLAG;
}

Flag_View::Flag_View(const Place &place, Index index_)
{
	index= index_;
	long_flag= place.bits & Place::Bits::LONG_FLAG;
}

void render(Flag_View fv, Parts &parts, Rendering rendering)
{
	assert(fv.index < C_ALL);
	if (fv.long_flag) {
		parts.append_marker("--");
		render(flag_info[fv.index].name, parts, rendering);
	} else {
		parts.append_marker("-");
		render(string(1, flag_chars[fv.index]), parts, rendering);
	}
}

void render(Unplaced_Flag_View ufv, Parts &parts, Rendering rendering)
{
	if (ufv.long_flag) {
		parts.append_marker("--");
		render(ufv.text, parts, rendering);
	} else {
		parts.append_marker("-");
		render(ufv.text, parts, rendering);
	}
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
			ret += flag_chars[i];
		}
	}
	if (ret.empty())
		return;
	ret= '-' + ret;
	parts.append_operator(ret);
}

#endif /* ! NDEBUG */
