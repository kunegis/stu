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
	/* The corresponding unparametrized target.  This target must have zero
	 * parameters. */
	{
		return Hash_Dep(flags, name.unparametrized());
	}
};

class Place_Target
/* A target that is parametrized and contains places.  Non-dynamic. */
{
public:
	Flags flags;  /* Only F_TARGET_PHONY is used */
	Place_Name place_name;

	Place place;
	/* The place of the target as a whole.  The PLACE_NAME variable additionally
	 * contains a place for the name itself, as well as for individual parameters. */

	Place_Target(Flags flags_,
		     const Place_Name &place_name_)
		: flags(flags_), place_name(place_name_), place(place_name_.place)
	{
		assert((flags_ & ~F_TARGET_PHONY) == 0);
	}

	Place_Target(Flags flags_,
		     const Place_Name &place_name_,
		     const Place &place_)
		: flags(flags_), place_name(place_name_), place(place_)
	{
		assert((flags_ & ~F_TARGET_PHONY) == 0);
	}

	Place_Target(const Place_Target &that)
		: flags(that.flags),
		  place_name(that.place_name),
		  place(that.place) { }

	bool equals_same_length(const Place_Target &that) const
	/* Compare, assuming same length */
	{
		return this->flags == that.flags &&
			this->place_name.equals_same_length(that.place_name);
	}

	void render(Parts &, Rendering) const;

	shared_ptr <Place_Target> instantiate(
		const std::map <string, string> &mapping) const {
		return std::make_shared <Place_Target>
			(flags, *place_name.instantiate(mapping), place);
	}

	Hash_Dep unparametrized() const {
		return Hash_Dep(flags, place_name.unparametrized());
	}

	Target get_target() const {
		return Target(flags, place_name);
	}

	void canonicalize();  /* In-place */
};

void render(const Place_Target &, Parts &, Rendering= 0);

#endif /* ! TARGET_HH */
