#ifndef DEP_HH
#define DEP_HH

/*
 * Dependencies are polymorphous objects, and all dependencies derive from class Dep, and
 * are used via shared_ptr<>.
 *
 * All dependency classes allow parametrized targets.
 *
 * A dependency can be normalized or not.  A dependency is normalized if it is one of:
 *    - a plain dependency (file or transient);
 *    - a dynamic dependency containing a normalized dependency;
 *    - a concatenated dependency of only normalized plain and dynamic dependencies.
 * In particular, compound dependencies are never normalized; they do not appear at all in
 * normalized dependencies.  Also, concatenated dependencies never contain other
 * concatenated dependencies directly -- such constructs are always "flattened" in a
 * normalized dependency.
 *
 * A plain dependency is a file or a transient.
 *
 * A dependency is simple when is does not involve concatenation or compound dependencies,
 * i.e., when it is a possible multiply dynamic dependency of a plain dependency.
 *
 * A dependency that is not simple is complex.  I.e., a complex dependency involves
 * concatenation and/or compound dependencies.
 */

/*
 * Functions such as Dep::normalize() cannot be virtual functions because we need to
 * access THIS as a shared pointer.
 */

#include <map>
#include <memory>

#include "target.hh"
#include "flags.hh"
#include "hints.hh"
#include "options.hh"
#include "show.hh"

template <typename T, typename U>
shared_ptr <const T> to(shared_ptr <const U> d)
{
	return std::dynamic_pointer_cast <const T> (d);
}

template <typename T, typename U>
shared_ptr <const T> to(shared_ptr <U> d)
{
	return std::dynamic_pointer_cast <const T> (d);
}

class Dep
	:  public std::enable_shared_from_this <Dep>
/*
 * The abstract base class for all dependencies.
 *
 * The flags only represent immediate flags.  Compound dependencies for instance may
 * contain additional inner flags.
 *
 * Objects of type Dep and subclasses are always handled through shared_ptr<>.  All
 * objects may have many persistent pointers to it, so they are considered final, i.e.,
 * immutable, except if we just created the object in which case we know that it is not
 * shared.  Therefore, we always use shared_ptr <const ...>, except when we just created
 * the dependency.  All dependencies are created via make_shared<>.
 *
 * The constructors of Dep and derived classes do not set the TOP and INDEX fields.  These
 * are set manually when needed.
 */
{
public:
	Flags flags;

	Place places[C_PLACED];
	/* For each transitive flag that is set, the place.  An empty place if a flag is
	 * not set. */

	shared_ptr <const Dep> top;
	/* Additional place used for constructing traces.  Most of the properties (such as
	 * extra flags) are ignored. */

	ssize_t index;
	/* Used by concatenated executors; the index of the dependency within the array of
	 * concatenation.  -1 when not used. */

	Dep(): flags(0), index(-1) { }
	Dep(Flags flags_): flags(flags_), index(-1) { }

	Dep(Flags flags_, const Place places_[C_PLACED])
		: flags(flags_), index(-1)
	{
		assert(places != places_);
		for (unsigned i= 0;  i < C_PLACED;  ++i)
			places[i]= places_[i];
	}

	Dep(const Dep &that)
		: std::enable_shared_from_this <Dep> (that),
		  flags(that.flags), top(that.top), index(that.index)
	{
		assert(this != &that);
		for (unsigned i= 0;  i < C_PLACED;  ++i)
			places[i]= that.places[i];
	}

	virtual ~Dep()= default;

	const Place &get_place_flag(unsigned i) const {
		assert(i < C_PLACED);
		return places[i];
	}

	Place &get_place_flag(unsigned i) {
		assert(i < C_PLACED);
		return places[i];
	}

	void set_place_flag(unsigned i, const Place &place) {
		assert(i < C_PLACED);
		places[i]= place;
	}

	void add_flags(shared_ptr <const Dep> dep,
		       bool overwrite_places);
	/* Add the flags from DEP.  Also copy over the corresponding places.  If a place
	 * is already given in THIS, only copy a place over if OVERWRITE_PLACES is set. */

	void normalize(
		std::vector <shared_ptr <const Dep> > &deps,
		int &error) const;
	/* Split DEP into multiple DEPS that are each normalized.  The resulting
	 * dependencies are appended to DEPS, which does not have to be empty on
	 * entering the function. On errors, a message is printed, bits are set
	 * in ERROR, and if not in keep-going mode, the function returns
	 * immediately. */

	shared_ptr <const Dep> untrivialize() const;
	/* Remove all trivial flags, recursively.  Return null if already trivialized. */

	shared_ptr <Dep> clone() const; /* Shallow clone */

	shared_ptr <const Dep> strip_dynamic() const;
	/* Strip dynamic dependencies from the given dependency.  Perform
	 * recursively:  If D is a dynamic dependency, return its contained
	 * dependency, otherwise return D.  Thus, never return null. */

#ifndef NDEBUG
	void check() const;
	/* The check function checks the internal consistency of a Dep object.  This is
	 * purely an assertion, and not a programmatic check.  It is possible for Dep
	 * objects to be temporarily inconsistent while they are changed -- therefore,
	 * consistency is not enforced by the accessor functions, but only by this
	 * function. */
#else
	void check() const { }
#endif

	virtual shared_ptr <const Dep> instantiate
	(const std::map <string, string> &mapping) const= 0;
	virtual bool is_unparametrized() const= 0;

	virtual const Place &get_place() const= 0;
	/* Where the dependency as a whole is declared */

	virtual void render(Parts &, Rendering= 0) const= 0;

	virtual Hash_Dep get_target() const= 0;
	/* Only called for non-compound and non-parametrized dependencies. */
#ifndef NDEBUG
	virtual bool is_normalized() const= 0;
#endif /* ! NDEBUG */

};

void render(shared_ptr <const Dep> dep, Parts &parts, Rendering rendering= 0)
{
	dep->render(parts, rendering);
}

class Plain_Dep
/* A dependency denoting an individual target name, which can be a file or a transient.
 *
 * When the target is a transient, the dependency flags have the F_TARGET_TRANSIENT bit
 * set, which is redundant, because that information is also contained in
 * PLACE_PARAM_TARGET.  No other Dep type has the F_TARGET_TRANSIENT flag set. */
	: public Dep
{
public:
	Place_Target place_target;
	/* The target of the dependency.  Has its own place, which may
	 * differ from the dependency's place, e.g. in '@all'.  Non-dynamic. */

	Place place;

	string variable_name;
	/* With F_VARIABLE:  the name of the variable.  Otherwise:  empty. */

	explicit Plain_Dep(const Place_Target &place_target_)
		: Dep(place_target_.flags),
		  place_target(place_target_),
		  place(place_target_.place)
	{
		check();
	}

	Plain_Dep(Flags flags_, const Place_Target &place_target_)
		/* Take the dependency place from the target place */
		: Dep(flags_), place_target(place_target_),
		  place(place_target_.place)
	{
		check();
	}

	Plain_Dep(Flags flags_,
		  const Place places_[C_PLACED],
		  const Place_Target &place_target_)
		/* Take the dependency place from the target place */
		: Dep(flags_, places_),
		  place_target(place_target_),
		  place(place_target_.place)
	{
		check();
	}

	Plain_Dep(
		Flags flags_,
		const Place_Target &place_target_,
		const Place &place_,
		std::string_view variable_name_)
		/* Use an explicit dependency place */
		: Dep(flags_),
		  place_target(place_target_),
		  place(place_),
		  variable_name(variable_name_)
	{
		check();
	}

	Plain_Dep(
		Flags flags_,
		const Place places_[C_PLACED],
		const Place_Target &place_target_,
		const Place &place_,
		std::string_view variable_name_)
		/* Use an explicit dependency place */
		: Dep(flags_, places_),
		  place_target(place_target_),
		  place(place_),
		  variable_name(variable_name_)
	{
		check();
	}

	Plain_Dep(
		Flags flags_,
		const Place places_[C_PLACED],
		const Place_Target &place_target_,
		std::string_view variable_name_)
		/* Use an explicit dependency place */
		: Dep(flags_, places_),
		  place_target(place_target_),
		  place(place_target_.place),
		  variable_name(variable_name_)
	{
		check();
	}

	Plain_Dep(const Plain_Dep &plain_dep)
		: Dep(plain_dep),
		  place_target(plain_dep.place_target),
		  place(plain_dep.place),
		  variable_name(plain_dep.variable_name) { }

	const Place &get_place() const override { return place; }
	virtual shared_ptr <const Dep> instantiate(const std::map <string, string> &mapping) const override;

	bool is_unparametrized() const override {
		return place_target.place_name.get_n() == 0;
	}

	virtual void render(Parts &, Rendering= 0) const override;

	virtual Hash_Dep get_target() const override;
	/* Does not preserve the F_VARIABLE bit */

#ifndef NDEBUG
	virtual bool is_normalized() const override { return true; }
#endif /* ! NDEBUG */
};

class Dynamic_Dep
/* The Dep::flags field has the F_TARGET_DYNAMIC set. */
	: public Dep
{
public:
	/* TODO rename to "dep_inner". */
	shared_ptr <const Dep> dep;
	/* The contained dependency.  Non-null. */

	Dynamic_Dep(shared_ptr <const Dep> dep_)
		/* Set the contained dependency.  Not a copy constructor. */
		: Dep(F_TARGET_DYNAMIC), dep(dep_)
	{ assert(dep_ != nullptr); }

	Dynamic_Dep(shared_ptr <const Dynamic_Dep> base_dep,
		    shared_ptr <const Dep> inner)
		: Dep(*base_dep), dep(inner) { }

	Dynamic_Dep(Flags flags_,
		    shared_ptr <const Dep> dep_)
		: Dep(flags_ | F_TARGET_DYNAMIC), dep(dep_)
	{
		assert((flags & F_VARIABLE) == 0);
		assert(dep_ != nullptr);
	}

	Dynamic_Dep(Flags flags_,
		    const Place places_[C_PLACED],
		    shared_ptr <const Dep> dep_)
		: Dep(flags_ | F_TARGET_DYNAMIC, places_), dep(dep_)
	{
		assert((flags & F_VARIABLE) == 0); /* Variables cannot be dynamic */
		assert(dep_ != nullptr);
	}

	virtual shared_ptr <const Dep> instantiate
	(const std::map <string, string> &mapping) const override;
	bool is_unparametrized() const override { return dep->is_unparametrized(); }

	const Place &get_place() const override
	/* In error message pointing to dynamic dependency such as '[B]', it is more
	 * useful to the user to point to the 'B' than to the '['. */
	{
		return dep->get_place();
	}

	virtual void render(Parts &, Rendering= 0) const override;
	virtual Hash_Dep get_target() const override;

	unsigned get_depth() const {
		if (to <Dynamic_Dep> (dep))
			return 1 + to <Dynamic_Dep> (dep)->get_depth();
		else
			return 1;
	}

#ifndef NDEBUG
	virtual bool is_normalized() const override { return dep->is_normalized(); }
#endif /* ! NDEBUG */
};

class Concat_Dep
/*
 * A dependency that is the concatenation of multiple dependencies.
 * The dependency as a whole does not have a place stored; the
 * place of the first sub-dependency is used.
 *
 * In terms of Stu code, a concatenated dependency corresponds to
 *
 *         ( X )( Y )( Z )...
 */
	: public Dep
{
public:
	std::vector <shared_ptr <const Dep> > deps;
	/* The dependencies for each part.  No entry is null.  May be empty in
	 * code, which is something that is not allowed in Stu code.  Otherwise,
	 * there are at least two elements. */

	Concat_Dep() { }
	/* An empty concatenation, i.e., a concatenation of zero dependencies */

	Concat_Dep(Flags flags_, const Place places_[C_PLACED])
		/* The list of dependencies is empty */
		: Dep(flags_, places_) { }

	Concat_Dep(shared_ptr <const Dep> dep)
		: Dep(*dep) { }

	/* Append a dependency to the list */
	void push_back(shared_ptr <const Dep> dep)
	{ deps.push_back(dep); }

	virtual shared_ptr <const Dep> instantiate
	(const std::map <string, string> &mapping) const override;

	virtual bool is_unparametrized() const override;
	virtual const Place &get_place() const override;
	virtual void render(Parts &, Rendering= 0) const override;
	virtual Hash_Dep get_target() const override;

	static shared_ptr <const Dep> concat(shared_ptr <const Dep> a,
					     shared_ptr <const Dep> b,
					     int &error);
	/* Concatenate two dependencies to a single dependency.  On error, a message is
	 * printed, bits are set in ERROR, and null is returned.  Only plain and dynamic
	 * dependencies can be passed. */

	static shared_ptr <const Plain_Dep> concat_plain(shared_ptr <const Plain_Dep> a,
							 shared_ptr <const Plain_Dep> b);
	static shared_ptr <const Concat_Dep> concat_complex(shared_ptr <const Dep> a,
							    shared_ptr <const Dep> b);

	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     std::vector <shared_ptr <const Dep> > &deps,
				     int &error);
	/* Normalize this object's dependencies into a list of individual dependencies.
	 * The generated dependencies are appended to DEPS which does not need to be empty
	 * on entry into this function.  On errors, a message is printed, bits are set in
	 * ERROR, and if not in keep-going mode, the function returns immediately. */

	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     std::vector <shared_ptr <const Dep> > &deps,
				     size_t start_index,
				     int &error);
	/* Helper function.  Write result into DEPS, concatenating all starting at the
	 * given index.  On errors, a message is printed, bits are set in ERROR, and if
	 * not in keep-going mode, the function returns immediately. */
#ifndef NDEBUG
	virtual bool is_normalized() const override;
#endif /* ! NDEBUG */
};

class Compound_Dep
/* A list of dependencies that act as a unit, corresponding syntactically to a list of
 * dependencies in parentheses.
 *
 * In terms of Stu source code, a compound dependency corresponds to
 *
 *         (X Y Z ...)
 *
 * Compound dependencies are themselves never normalized.  Within normalized dependencies,
 * they appear only as immediate children of concatenated dependencies.  Otherwise, they
 * also appear after parsing to denote syntactic groups of dependencies. */
	: public Dep
{
public:
	Place place;
	/* The place of the compound ; usually the opening parenthesis or brace.  May be
	 * empty to denote no place, in particular if this is a "logical" compound
	 * dependency not coming from a parenthesised expression. */

	std::vector <shared_ptr <const Dep> > deps;
	/* The contained dependencies, in given order */

	Compound_Dep(const Place &place_)
		/* Empty, with zero dependencies */
		: place(place_) { }

	Compound_Dep(Flags flags_, const Place places_[C_PLACED], const Place &place_)
		: Dep(flags_, places_), place(place_)
	{ /* The list of dependencies is empty */ }

	Compound_Dep(std::vector <shared_ptr <const Dep> > &&deps_,
		     const Place &place_)
		: place(place_), deps(deps_) { }

	void push_back(shared_ptr <const Dep> dep) { deps.push_back(dep); }

	virtual shared_ptr <const Dep> instantiate(const std::map <string, string> &mapping) const override;
	virtual bool is_unparametrized() const override;
	virtual const Place &get_place() const override { return place; }
	virtual void render(Parts &, Rendering= 0) const override;
	virtual Hash_Dep get_target() const override { unreachable(); }
#ifndef NDEBUG
	virtual bool is_normalized() const override { return false; }
#endif /* ! NDEBUG */
};

class Root_Dep
/* Dependency to denote the root object of the dependency tree.  There is just one
 * possible value of this, and it is never shown to the user, but used internally with the
 * root executor object. */
	: public Dep
{
public:
	virtual shared_ptr <const Dep> instantiate(const std::map <string, string> &) const override {
		return shared_ptr <const Dep> (std::make_shared <Root_Dep> ());
	}
	virtual bool is_unparametrized() const override { return false; }
	virtual const Place &get_place() const override { return Place::place_empty; }
	virtual void render(Parts &parts, Rendering= 0) const override;
	virtual Hash_Dep get_target() const override { unreachable(); }
#ifndef NDEBUG
	virtual bool is_normalized() const override { return true; }
#endif /* ! NDEBUG */
};

#endif /* ! DEP_HH */
