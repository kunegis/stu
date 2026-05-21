#include "placed_flags.hh"

#include <algorithm>

void Placed_Flags::add_unplaced_flags(Flags flags_new)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags_new= %s", show(Flags_View(flags_new)));
	assert((flags_new & ~F_UNPLACED) == 0);
	flags |= flags_new;
	TRACE("flags= %s", show(Flags_View(flags)));
}

void Placed_Flags::remove_unplaced_flags(Flags flags_remove)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags_remove= %s", show(Flags_View(flags_remove)));
	assert((flags_remove & ~F_UNPLACED) == 0);
	flags &= ~flags_remove;
	TRACE("flags= %s", show(Flags_View(flags)));
}

void Placed_Flags::add_unplaced_index(Index index)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show(Flags_View(1 << index)));
	assert(index < C_ALL);
	assert((1 << index) & F_UNPLACED);
	flags |= 1 << index;
}

void Placed_Flags::add_placed_index(Index index, const Place &place)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show(Flags_View(1 << index)));
	TRACE("flags= %s", show(Flags_View(flags)));
	assert(index < C_ALL);
	assert((1 << index) & F_PLACED);
	for (size_t i= 0; i < placed_flags.size(); ++i) {
		if (placed_flags[i].index == index) {
			TRACE("Already present");
			return;
		}
	}
	TRACE("Appending");
	placed_flags.emplace_back(index, place);
	flags |= 1 << index;

	TRACE("flags= %s", show(Flags_View(flags)));
}

const Place &Placed_Flags::place_by_index(Index index) const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("index= %s", show(Flags_View(1 << index)));
	assert(index < C_ALL);
	assert((1 << index) & F_PLACED);
	for (size_t i= 0; i < placed_flags.size(); ++i) {
		if (placed_flags[i].index == index)
			return placed_flags[i].place;
	}
	should_not_happen();
	return Place::place_empty;
}

void Placed_Flags::add(const Placed_Flags &that, Flags filter)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("that= %s", frmt("%p", (void *)&that));
	TRACE("filter= %s", show(Flags_View(filter & F_ALL)));
	TRACE("this->flags= %s", show(Flags_View(flags)));
	TRACE("that.flags= %s", show(Flags_View(that.flags)));
	check();
	that.check();
	for (size_t i= 0; i < that.placed_flags.size(); ++i) {
		TRACE("i= %s", frmt("%zu", i));
		if ((flags | ~filter) & (1 << that.placed_flags[i].index)) {
			continue;
		}
		placed_flags.push_back(that.placed_flags[i]);
	}
	flags |= that.flags & filter;
	TRACE("this->flags= %s", show(Flags_View(flags)));
}

void Placed_Flags::remove_index(Index index)
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags= %s", show(Flags_View(flags)));
	TRACE("index= %s", show(Flags_View(1 << index)));
	assert(index < C_ALL);

	if (!(flags & (1 << index))) return;
	TRACE("Flag is present");

	if ((1 << index) & F_PLACED) {
		TRACE("Flag is placed");
		auto f= std::find_if(placed_flags.begin(), placed_flags.end(),
			[index](const Placed_Flag &pf){
				return pf.index == index; });
		if (f != placed_flags.end()) {
			size_t i= std::distance(placed_flags.begin(), f);
			TRACE("i= %s", frmt("%zu", i));
			TRACE("size= %s", frmt("%zu", placed_flags.size()));
			if (i+1 != placed_flags.size()) {
				TRACE("Found item is non-last; swap needed");
				std::swap(placed_flags[i],
					placed_flags[placed_flags.size() - 1]);
			} else {
				TRACE("Found item is last; no swap needed");
			}
			placed_flags.resize(placed_flags.size() - 1);
		} else {
			TRACE("Flag not found");
			unreachable();
		}
	}

	flags &= ~(1 << index);

	TRACE("flags= %s", show(Flags_View(flags)));
	TRACE("placed_flags.size()= %s", frmt("%zu", placed_flags.size()));
	check();
}

#ifndef NDEBUG

void Placed_Flags::check() const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	TRACE("flags= %s", show(Flags_View(flags)));
	assert((flags & ~F_ALL) == 0);
	Flags f= 0;

	for (size_t i= 0; i < placed_flags.size(); ++i) {
		Index index= placed_flags[i].index;
		TRACE("i= %s; index= %s",
			frmt("%zu", i),
			show(Flags_View(1 << index)));
		assert(flags & (1 << index));
		assert((f & (1 << index)) == 0);
		f |= 1 << index;
	}

	assert(f == (flags & F_PLACED));
}

bool Placed_Flags::contains_index(Index index) const
{
	TRACE_FUNCTION();
	TRACE("this= %s", frmt("%p", (void *)this));
	assert(index < C_ALL);
	return (1 << index) & flags;
}

#endif /* ! NDEBUG */
