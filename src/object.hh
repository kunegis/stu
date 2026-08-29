#ifndef OBJECT_HH
#define OBJECT_HH

#include "flags.hh"
#include "hash_bare_dep.hh"
#include "name.hh"
#include "place.hh"
#include "show.hh"

class Object
/* A parametrized name for which it is saved what type it represents.  Non-dynamic. */
{
public:
	Flags flags;  /* Only file/phony info */
	Name name;

	Object(Flags flags_, const Name &name_)
		: flags(flags_), name(name_)
	{
		assert((flags_ & ~F_PHONY) == 0);
	}

	Object(Hash_Bare_Dep hash_bare_dep)
	/* Unparametrized object. The passed Hash_Plain_Dep must be non-dynamic. */
		: flags(hash_bare_dep.get_front_word_nondynamic() & F_PHONY),
		  name(hash_bare_dep.get_name_nondynamic())
	{
		assert(! hash_bare_dep.is_dynamic());
	}

	Hash_Bare_Dep instantiate(const std::map <string, string> &mapping) const {
		return Hash_Bare_Dep(flags, name.instantiate(mapping));
	}

	Hash_Bare_Dep unparametrized() const
	/* The corresponding unparametrized object.  Must have zero parameters. */
	{
		return Hash_Bare_Dep(flags, name.unparametrized());
	}
};

class Placed_Object
/* A object that is parametrized and contains places.  Non-dynamic. */
{
public:
	Flags flags;  /* Only F_TARGET_PHONY is used */
	Placed_Name name;

	Place place;
	/* The place of the object as a whole.  The PLACED_NAME variable additionally
	 * contains a place for the name itself, as well as for individual parameters. */

	Placed_Object(
		Flags flags_,
		const Placed_Name &name_)
		: flags(flags_), name(name_), place(name_.place)
	{
		assert((flags_ & ~F_PHONY) == 0);
	}

	Placed_Object(
		Flags flags_,
		const Placed_Name &name_,
		const Place &place_)
		: flags(flags_), name(name_), place(place_)
	{
		assert((flags_ & ~F_PHONY) == 0);
	}

	Placed_Object(const Placed_Object &that)
		: flags(that.flags),
		  name(that.name),
		  place(that.place) { }

	bool equals_same_length(const Placed_Object &that) const
	/* Compare, assuming same length */
	{
		return this->flags == that.flags &&
			this->name.equals_same_length(that.name);
	}

	void render(Parts &, Rendering) const;

	shared_ptr <Placed_Object> instantiate(
		const std::map <string, string> &mapping) const;

	Hash_Bare_Dep unparametrized() const {
		return Hash_Bare_Dep(flags, name.unparametrized());
	}

	Object get_Object() const {
		return Object(flags, name);
	}

	void canonicalize();  /* In-place */
};

void render(const Placed_Object &, Parts &, Rendering= 0);

#endif /* ! OBJECT_HH */
