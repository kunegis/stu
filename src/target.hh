#ifndef TARGET_HH
#define TARGET_HH

/*
 * Targets are to be distinguished from the more general dependencies, which can
 * represent any nested expression, including concatenations, flags, compound
 * expressions, etc., while targets only represent individual files or phonies.
 */

#include "flags.hh"
#include "hash_dep.hh"
#include "name.hh"
#include "place.hh"
#include "show.hh"

class Target
/* A parametrized name for which it is saved what type it represents.  Non-dynamic. */
{
public:
	Flags flags;  /* Only file/phony target info */
	Name name;

	Target(Flags flags_, const Name &name_)
		: flags(flags_), name(name_)
	{
		assert((flags_ & ~F_TARGET_PHONY) == 0);
	}

	Target(Hash_Dep hash_dep)
	/* Unparametrized target. The passed TARGET must be non-dynamic. */
		: flags(hash_dep.get_front_word_nondynamic() & F_TARGET_PHONY),
		  name(hash_dep.get_name_nondynamic())
	{
		assert(! hash_dep.is_dynamic());
	}

	Hash_Dep instantiate(const std::map <string, string> &mapping) const {
		return Hash_Dep(flags, name.instantiate(mapping));
	}

	Hash_Dep unparametrized() const
	/* The corresponding unparametrized target.  Must have zero parameters. */
	{
		return Hash_Dep(flags, name.unparametrized());
	}
};

class Placed_Target
/* A target that is parametrized and contains places.  Non-dynamic. */
{
public:
	Flags flags;  /* Only F_TARGET_PHONY is used */
	Placed_Name placed_name;

	Place place;
	/* The place of the target as a whole.  The PLACED_NAME variable additionally
	 * contains a place for the name itself, as well as for individual parameters. */

	Placed_Target(Flags flags_,
		const Placed_Name &placed_name_)
		: flags(flags_), placed_name(placed_name_), place(placed_name_.place)
	{
		assert((flags_ & ~F_TARGET_PHONY) == 0);
	}

	Placed_Target(Flags flags_,
		const Placed_Name &placed_name_,
		const Place &place_)
		: flags(flags_), placed_name(placed_name_), place(place_)
	{
		assert((flags_ & ~F_TARGET_PHONY) == 0);
	}

	Placed_Target(const Placed_Target &that)
		: flags(that.flags),
		  placed_name(that.placed_name),
		  place(that.place) { }

	bool equals_same_length(const Placed_Target &that) const
	/* Compare, assuming same length */
	{
		return this->flags == that.flags &&
			this->placed_name.equals_same_length(that.placed_name);
	}

	void render(Parts &, Rendering) const;

	shared_ptr <Placed_Target> instantiate(
		const std::map <string, string> &mapping) const {
		return std::make_shared <Placed_Target>
			(flags, *placed_name.instantiate(mapping), place);
	}

	Hash_Dep unparametrized() const {
		return Hash_Dep(flags, placed_name.unparametrized());
	}

	Target get_target() const {
		return Target(flags, placed_name);
	}

	void canonicalize();  /* In-place */
};

void render(const Placed_Target &, Parts &, Rendering= 0);

#endif /* ! TARGET_HH */
