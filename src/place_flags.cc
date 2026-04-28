#include "place_flags.hh"

#include <algorithm>

void Place_Flags::add_unplaced_flags(Flags flags_new)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags_new= %s", show_flags(flags_new));
	assert((flags_new & ~F_UNPLACED) == 0);
	flags |= flags_new;
	TRACE("flags= %s", show_flags(flags));
}

void Place_Flags::remove_unplaced_flags(Flags flags_remove)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags_remove= %s", show_flags(flags_remove));
	assert((flags_remove & ~F_UNPLACED) == 0);
	flags &= ~flags_remove;
	TRACE("flags= %s", show_flags(flags));
}

void Place_Flags::add_unplaced_index(Index index)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show_flags(1 << index));
	assert(index < C_ALL);
	assert((1 << index) & F_UNPLACED);
	flags |= 1 << index;
}

void Place_Flags::add_placed_index(Index index, const Place &place)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show_flags(1 << index));
	TRACE("flags= %s", show_flags(flags));
	assert(index < C_ALL);
	assert((1 << index) & F_PLACED);
	for (size_t i= 0; i < place_flags.size(); ++i) {
		if (place_flags[i].index == index) {
			TRACE("Already present");
			return;
		}
	}
	TRACE("Appending");
	place_flags.emplace_back(index, place);
	flags |= 1 << index;

	TRACE("flags= %s", show_flags(flags));
}

const Place &Place_Flags::place_by_index(Index index) const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show_flags(1 << index));
	assert(index < C_ALL);
	assert((1 << index) & F_PLACED);
	for (size_t i= 0; i < place_flags.size(); ++i) {
		if (place_flags[i].index == index)
			return place_flags[i].place;
	}
	should_not_happen();
	return Place::place_empty;
}

void Place_Flags::add(const Place_Flags &that, Flags filter)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("that= %s", frmt("%p", (void *)&that));
	TRACE("filter= %s", show_flags(filter & F_ALL));
	TRACE("this->flags= %s", show_flags(flags));
	TRACE("that.flags= %s", show_flags(that.flags));
	check();
	that.check();
	for (size_t i= 0; i < that.place_flags.size(); ++i) {
		TRACE("i= %s", frmt("%zu", i));
		if ((flags | ~filter) & (1 << that.place_flags[i].index)) {
			continue;
		}
		place_flags.push_back(that.place_flags[i]);
	}
	flags |= that.flags & filter;
	TRACE("this->flags= %s", show_flags(flags));
}

void Place_Flags::remove_index(Index index)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags= %s", show_flags(flags));
	TRACE("index= %s", show_flags(1 << index));
	assert(index < C_ALL);

	if (!(flags & (1 << index))) return;
	TRACE("Flag is present");

	if ((1 << index) & F_PLACED) {
		TRACE("Flag is placed");
		auto f= std::find_if(place_flags.begin(), place_flags.end(),
			[index](const Place_Flag &pf){
				return pf.index == index; });
		if (f != place_flags.end()) {
			size_t i= std::distance(place_flags.begin(), f);
			TRACE("i= %s", frmt("%zu", i));
			TRACE("size= %s", frmt("%zu", place_flags.size()));
			if (i+1 != place_flags.size()) {
				TRACE("Found item is non-last; swap needed");
				std::swap(place_flags[i],
					place_flags[place_flags.size() - 1]);
			} else {
				TRACE("Found item is last; no swap needed");
			}
			place_flags.resize(place_flags.size() - 1);
		} else {
			TRACE("Flag not found");
			unreachable();
		}
	}

	flags &= ~(1 << index);

	TRACE("flags= %s", show_flags(flags));
	TRACE("place_flags.size()= %s", frmt("%zu", place_flags.size()));
	check();
}

#ifndef NDEBUG

void Place_Flags::check() const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags= %s", show_flags(flags));
	assert((flags & ~F_ALL) == 0);
	Flags f= 0;

	for (size_t i= 0; i < place_flags.size(); ++i) {
		Index index= place_flags[i].index;
		TRACE("i= %s; index= %s",
			frmt("%zu", i),
			show_flags(1 << index));
		assert(flags & (1 << index));
		assert((f & (1 << index)) == 0);
		f |= 1 << index;
	}

	assert(f == (flags & F_PLACED));
}

bool Place_Flags::contains_index(Index index) const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	assert(index < C_ALL);
	return (1 << index) & flags;
}

#endif /* ! NDEBUG */
