#include "state.hh"

#ifndef NDEBUG

void render(State state, Parts &parts, Rendering)

{
	static const char *state_text[]= {
		"NEED_BUILD", "CHECKED", "EXISTING", "MISSING"
	};
	static_assert(State::COUNT == sizeof(state_text) / sizeof(state_text[0]));

	bool first= true;
	for (int i= 0; i < State::COUNT; ++i) {
		if (state & State(1 << i)) {
			if (! first)
				parts.append_operator("|");
			first= false;
			parts.append_text(state_text[i]);
		}
	}
	if (first)
		parts.append_text("0");
}

#endif /* ! NDEBUG */
