#ifndef SHOW_FLAGS_HH
#define SHOW_FLAGS_HH

#include "show.hh"
#include "placed_flags.hh"

class Flag_View
{
public:
	Flag_View(Index i, bool long_flag_): index(i), long_flag(long_flag_) {}
	Flag_View(const Placed_Flags &, Index index_);
	Flag_View(const Place &, Index index_);
private:
	friend void render(Flag_View, Parts &, Rendering);
	Index index;
	bool long_flag;
};

void render(Flag_View, Parts &, Rendering= 0);

class Unplaced_Flag_View
{
public:
	Unplaced_Flag_View(string text_, bool long_flag_)
		: text(text_), long_flag(long_flag_) {}
	Unplaced_Flag_View(char c): text(1, c), long_flag(false) {}
private:
	friend void render(Unplaced_Flag_View, Parts &, Rendering);
	string text;
	bool long_flag;
};

void render(Unplaced_Flag_View, Parts &, Rendering= 0);

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
