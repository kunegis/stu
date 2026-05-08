#ifndef SHOW_FLAGS_HH
#define SHOW_FLAGS_HH

#include "show.hh"

class Flag_View
{
public:
	char c;
	Flag_View(char c_): c(c_) {}
};

void render(Flag_View, Parts &, Rendering= 0);

#ifndef NDEBUG

class Flags_View
{
public:
	Flags flags;
	Flags_View(Flags flags_): flags(flags_) {}
};

void render(Flags_View, Parts &, Rendering= 0);

#endif /* ! NDEBUG */

#endif /* ! SHOW_FLAGS_HH */
