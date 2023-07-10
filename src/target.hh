#ifndef TARGET_HH
#define TARGET_HH

/*
 * Targets are to be distinguished from the more general dependencies, which can
 * represent any nested expression, including concatenations, flags, compound
 * expressions, etc., while targets only represent individual files or
 * transients.
 */

#include "error.hh"
#include "flags.hh"
#include "hash_dep.hh"
#include "show.hh"

class Name
/* The possibly parametrized name of a file or transient.  A name has N >= 0
 * parameters.  When N > 0, the name is parametrized, otherwise it is
 * unparametrized.  A name consists of N+1 static text elements (in the variable
 * TEXTS) and N parameters (in PARAMETERS), which are interleaved.  For instance
 * when N = 2, the name is given by
 *
 *	texts[0] parameters[0] texts[1] parameters[1] texts[2].
 *
 * Names can be valid or invalid.  A name is valid when all internal texts
 * (between two parameters) are non-empty, and, if N = 0, the single text is
 * non-empty.  A name is empty if N = 0 and the single text is empty (empty
 * names are invalid). */
{
public:
	Name(string name_): texts({name_}) { }
	/* A name with zero parameters */

	Name(): texts({""}) { }
	/* Empty name */

	bool empty() const {
		assert(texts.size() == 1 + parameters.size());
		return parameters.empty() && texts[0].empty();
	}

	size_t get_n() const
	/* Number of parameters; zero when the name is unparametrized. */
	{
		assert(texts.size() == 1 + parameters.size());
		return parameters.size();
	}

	bool is_parametrized() const {
		assert(texts.size() == 1 + parameters.size());
		return !parameters.empty();
	}

	const std::vector <string> &get_texts() const {
		return texts;
	}

	const std::vector <string> &get_parameters() const {
		return parameters;
	}

	void append_parameter(string parameter)
	/* Append the given PARAMETER and an empty text.  This does not
	 * check that the result is valid.  */
	{
		parameters.push_back(parameter);
		texts.push_back("");
	}

	void append_text(string text)
	/* Append the given text to the last text element */
	{
		texts[texts.size() - 1] += text;
	}

	void append(const Name &name);
	/* Append another parametrized name.  Check that the result is valid. */

	string &last_text() {
		return texts[texts.size() - 1];
	}
	const string &last_text() const {
		return texts[texts.size() - 1];
	}

	string instantiate(const std::map <string, string> &mapping) const;
	/* Instantiate the name with the given mapping.  The name may be
	 * empty, resulting in an empty string.  */

	const string &unparametrized() const
	/* Return the name as a string, assuming it is unparametrized.
	 * The name must be unparametrized.  */
	{
		assert(get_n() == 0);
		return texts[0];
	}

	bool match(string name, std::map <string, string> &mapping,
		   std::vector <size_t> &anchoring, int &priority) const;
	/* Check whether NAME matches this name.  If it does, return
	 * TRUE and set MAPPING and ANCHORING accordingly.
	 * MAPPING must be empty.  PRIORITY determines whether a special rule was used:
	 *    0:   no special rule was used
	 *    +1:  a special rule was used, having priority of matches without special rule
	 *    -1:  a special rule was used, having less priority than matches
	 *         without special rule
	 * PRIORITY has an unspecified value after returing FALSE.
	 * The range of PRIORITY can be easily extended to other integers if necessary.
	 */

	void render(Parts &, Rendering= 0) const;

	string get_duplicate_parameter() const;
	/* Check whether there are duplicate parameters.  Return the
	 * name of the found duplicate parameter, or "" if none is found.  */

	bool valid(string &param_1, string &param_2) const;
	/* Whether this is a valid name.  If it is not, fill the given
	 * parameters with the two unseparated parameters.  */

	void canonicalize();
	/* In-place canonicalizarion */

	bool operator==(const Name &that) const;

	static bool anchoring_dominates(std::vector <size_t> &anchoring_a,
					std::vector <size_t> &anchoring_b,
					int priority_a, int priority_b);
	/* Whether anchoring A dominates anchoring B.  The anchorings do
	 * not need to have the same number of parameters.  */

private:
	std::vector <string> texts; /* Length = N + 1 */
	std::vector <string> parameters; /* Length = N */
};

void render(const Name &name, Parts &parts, Rendering rendering= 0)
{
	name.render(parts, rendering);
}

class Target
/* A parametrized name for which it is saved what type it represents.
 * Non-dynamic.  */
{
public:
	Flags flags;  /* Only file/transient target info */
	Name name;

	Target(Flags flags_, const Name &name_)
		: flags(flags_), name(name_)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Target(Hash_Dep hash_dep)
	/* Unparametrized target. The passed TARGET must be non-dynamic. */
		: flags(hash_dep.get_front_word_nondynamic() & F_TARGET_TRANSIENT),
		  name(hash_dep.get_name_nondynamic())
	{
		assert(! hash_dep.is_dynamic());
	}

	Hash_Dep instantiate(const std::map <string, string> &mapping) const {
		return Hash_Dep(flags, name.instantiate(mapping));
	}

	Hash_Dep unparametrized() const
	/* The corresponding unparametrized target.  This target must
	 * have zero parameters.  */
	{
		return Hash_Dep(flags, name.unparametrized());
	}

	bool operator== (const Target &that) const {
		return this->flags == that.flags &&
			this->name == that.name;
	}

	bool operator!= (const Target &that) const {
		return ! (*this == that);
	}
};

class Place_Name
/* A possibly parametrized name annotated with places */
	:  public Name
{
public:
	Place place;
	/* Place of the name as a whole */

	std::vector <Place> places;
	/* Length = N (number of parameters).
	 * The places of the individual parameters.  */

	Place_Name()
		/* Empty parametrized name, and empty place */
		: Name(), place() { }

	Place_Name(string name)
		/* Unparametrized, with empty place */
		: Name(name)
	{
		/* PLACES remains empty */
	}

	Place_Name(string name, const Place &_place)
		/* Unparametrized, with explicit place */
		: Name(name), place(_place)
	{
		assert(! place.empty());
	}

	const std::vector <Place> &get_places() const {
		return places;
	}

	void append_parameter(string parameter,
			      const Place &place_parameter)
	/* Append the given PARAMETER and an empty text */
	{
		Name::append_parameter(parameter);
		places.push_back(place_parameter);
	}

	shared_ptr <Place_Name> instantiate(const std::map <string, string> &mapping) const
	/* In the returned object, the PLACES vector is empty */
	{
		string name= Name::instantiate(mapping);
		return std::make_shared <Place_Name> (name, place);
	}
};

void show(Parts &, const Place_Name &place_name);

class Place_Target
/* A target that is parametrized and contains places.  Non-dynamic. */
{
public:
	Flags flags;  /* Only F_TARGET_TRANSIENT is used */
	Place_Name place_name;

	Place place;
	/* The place of the target as a whole.  The PLACE_NAME
	 * variable additionally contains a place for the name itself,
	 * as well as for individual parameters.  */

	Place_Target(Flags flags_,
		     const Place_Name &place_name_)
		: flags(flags_), place_name(place_name_), place(place_name_.place)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Place_Target(Flags flags_,
		     const Place_Name &place_name_,
		     const Place &place_)
		: flags(flags_), place_name(place_name_), place(place_)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Place_Target(const Place_Target &that)
		: flags(that.flags),
		  place_name(that.place_name),
		  place(that.place) { }

	bool operator==(const Place_Target &that) const
	/* Compares only the content, not the place. */
	{
		return this->flags == that.flags &&
			this->place_name == that.place_name;
	}

	void render(Parts &, Rendering) const;

	shared_ptr <Place_Target>
	instantiate(const std::map <string, string> &mapping) const {
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

	static shared_ptr <Place_Target> clone
	(shared_ptr <const Place_Target> place_target) {
		shared_ptr <Place_Target> ret= std::make_shared <Place_Target>
			(place_target->flags,
			 place_target->place_name,
			 place_target->place);
		return ret;
	}
};

void render(const Place_Target &place_target,
	    Parts &parts, Rendering rendering= 0)
{
	return place_target.render(parts, rendering);
}

shared_ptr <const Place_Target> canonicalize(shared_ptr <const Place_Target> );

#endif /* ! TARGET_HH */
