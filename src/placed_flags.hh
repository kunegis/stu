#ifndef PLACED_FLAGS_HH
#define PLACED_FLAGS_HH

#include "place.hh"

class Placed_Flag
{
public:
	Place place;
	Index index;

	Placed_Flag() {} /* uncovered */
	Placed_Flag(Index i, const Place &p): place(p), index(i) {}
};

class Placed_Flags
{
public:
	Flags get_flags() const { return flags; }
	void add_unplaced_flags(Flags flags_new);
	void remove_unplaced_flags(Flags flags_remove);
	const std::vector <Placed_Flag> get() const { return placed_flags; }
	const Place &place_by_index(Index index) const;
	void add_unplaced_index(Index index);
	void add_placed_index(Index index, const Place &place);
	void add(const Placed_Flags &, Flags filter= ~(Flags)0);
	void remove_index(Index index); /* Flag need not be present */

#ifdef NDEBUG
	void check() const {}
#else
	void check() const;
	bool contains_index(Index index) const;
#endif

private:
	Flags flags= 0;
	std::vector <Placed_Flag> placed_flags;
};

#endif /* ! PLACED_FLAGS_HH */
