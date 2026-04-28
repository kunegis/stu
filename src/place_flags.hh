#ifndef PLACE_FLAGS_HH
#define PLACE_FLAGS_HH

#include "place.hh"

class Place_Flag
{
public:
	Place place;
	Index index;

	Place_Flag() {} /* uncovered */
	Place_Flag(Index i, const Place &p)
		: place(p), index(i) {}
};

class Place_Flags
{
public:
	Flags get_flags() const { return flags; }
	void add_unplaced_flags(Flags flags_new);
	void remove_unplaced_flags(Flags flags_remove);
	const std::vector <Place_Flag> get() const { return place_flags; }
	const Place &place_by_index(Index index) const;
	void add_unplaced_index(Index index);
	void add_placed_index(Index index, const Place &place);
	void add(const Place_Flags &, Flags filter= ~(Flags)0);
	void remove_index(Index index); /* Flag need not be present */

#ifdef NDEBUG
	void check() const {}
#else
	void check() const;
	bool contains_index(Index index) const;
#endif

private:
	Flags flags= 0;
	std::vector <Place_Flag> place_flags;
};

#endif /* ! PLACE_FLAGS_HH */
