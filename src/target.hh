#ifndef TARGET_HH
#define TARGET_HH

/*
 * Targets are the individual "objects" of Stu.  They can be thought of
 * as the "native types" of Stu.
 *
 * Targets can be either files or transients, and can have any level of
 * dynamicity.
 *
 * Targets are to be distinguished from the more general dependencies,
 * which can represent any nested expression, including concatenations,
 * flags, compound expressions, etc., while targets only represent
 * individual files or transients.
 */

/*
 * Glossary:
 * * A _name_ is a filename or the name of a transient target.  They are just
 *   strings, so no special data type is used for them.  There are two distinct
 *   namespaces for them (files and transients.)  They can contain any character
 *   except \0, and must not be the empty string.
 * * A _target_ is either file, transient target, or a dynamic^* of them.  It is
 *   represented by a name (string) and a type.  A _parametrized_ target or name
 *   additionally can have parameters.
 * * Dedicated classes exist to represent these with _places_.
 * * An _anchoring_ is the information about which part of a string matches the
 *   parameters of a target.  An anchoring is represented as a vector of
 *   integers, the length of which is twice the number of parameters.  Example:
 *   When name.$a.$b is matched to the name 'name.xxx.7', then the anchoring is
 *   [5, 8, 9, 10]. 
 */

#include <assert.h>
#include <vector>

#include "error.hh"
#include "flags.hh"
#include "show.hh"

#if   C_WORD <= 8
typedef uint8_t  word_t;
#elif C_WORD <= 16
typedef uint16_t word_t;
#else
#	error "Invalid word size"
#endif

class Target
/* A representation of a simple dependency, mainly used as the key in
 * the caching of Executor objects.  The difference to the Dependency
 * class is that Target objects don't store the Place objects, and don't
 * support parametrization.  Thus, Target objects are used as keys in
 * maps, etc.  Flags are included.
 *
 * TEXT is a linear representation of the target.  It begins with a certain
 * number of words (word_t, at least one), followed by the name of the target as
 * a string.  A word_t is represented by a fixed number of characters.
 *
 * A non-dynamic dependency is represented as a Type word (F_TARGET_TRANSIENT or
 * 0) followed by the name.  
 *
 * A dynamic is represented as a dynamic word (F_TARGET_DYNAMIC) followed by the
 * string representation of the contained dependency.
 *
 * Any of the front words may contain additional flag bits.  There may be '\0'
 * bytes in the front words, but the name does not contain nul, as that is
 * invalid in names.  The name proper (excluding front words) is non-empty,
 * i.e., is at least one byte long.
 *
 * The empty string denotes a "null" value for the type Target, or equivalently
 * the target of the root dependency, in which case most functions should not be
 * used.  */
{
public:
	Target()
		/* The "null" target */
		:  Target("")  {  }

	explicit Target(string text_)
		/* TEXT_ is the full text field of this Target */
		:  text(text_)  {  }

	Target(Flags flags, string name)
	/* A plain target */
		: text(string_from_word(flags) + name)
	{
		assert((flags & ~F_TARGET_TRANSIENT) == 0);
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(! name.empty());
	}

	Target(Flags flags, Target target)
	/* Makes the given target once more dynamic with the given
	 * flags, which must *not* contain the 'dynamic' flag.  */
		:  text(string_from_word(flags | F_TARGET_DYNAMIC) + target.text)
	{
		assert((flags & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0);
		assert(flags <= (unsigned)(1 << C_WORD));
	}

	const string &get_text() const {  return text;  }
	string &get_text() {  return text;  }
	const char *get_text_c_str() const {  return text.c_str();  }

	bool is_dynamic() const {
		check();
		return get_word(0) & F_TARGET_DYNAMIC;
	}

	bool is_file() const {
		check();
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0;
	}

	bool is_transient() const {
		check();
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT))
			== F_TARGET_TRANSIENT;
	}

	bool is_any_file() const {
		size_t i= 0;
		while (get_word(i) & F_TARGET_DYNAMIC) {
			++i;
		}
		return (get_word(i) & F_TARGET_TRANSIENT) == 0;
	}

	bool is_any_transient() const {
		size_t i= 0;
		while (get_word(i) & F_TARGET_DYNAMIC) {
			++i;
		}
		return get_word(i) & F_TARGET_TRANSIENT;
	}

	void render(Parts &, Rendering= 0) const;

	string get_name_nondynamic() const
	/* Get the name of the target, knowing that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return text.substr(sizeof(word_t));
	}

	const char *get_name_c_str_nondynamic() const
	/* Return a C pointer to the name of the file or transient.  The
	 * object must be non-dynamic.  */
	{
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return text.c_str() + sizeof(word_t);
	}

	const char *get_name_c_str_any() const
	{
		const char *ret= text.c_str();
		while ((*(const word_t *)ret) & F_TARGET_DYNAMIC)
			ret += sizeof(word_t);
		return
			ret += sizeof(word_t);
	}

	Flags get_front_word() const {  return get_word(0);  }

	word_t &get_front_word_nondynamic()
	/* Get the front byte, given that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return *(word_t *)&text[0];
	}

	Flags get_front_word_nondynamic() const {
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return *(const word_t *)&text[0];
	}

	Flags get_word(size_t i) const
	/* For access to any front word */
	{
		assert(text.size() > sizeof(word_t) * (i + 1));
		return ((const word_t *)&text[0])[i];
	}

	bool operator==(const Target &target) const {  return text == target.text;  }
	bool operator!=(const Target &target) const {  return text != target.text;  }
	void canonicalize();  /* In-place */

	static string string_from_word(Flags flags)
	/* Return a string of length sizeof(word_t) containing the given
	 * flags  */
	{
		assert(flags <= 1 << C_WORD);
		char ret[sizeof(word_t) + 1];
		ret[sizeof(word_t)]= '\0';
		*(word_t *)ret= (word_t)flags;
		return string(ret, sizeof(word_t));
	}

private:
	string text;

	void check() const {
		/* The minimum length of TEXT is sizeof(word_t)+1:  One
		 * word indicating a non-dynamic target, and a text of
		 * length one.  (The text cannot be empty.)  */
#ifndef NDEBUG
		assert(text.size() > sizeof(word_t));
#endif /* ! NDEBUG */
	}
};

void render(const Target &target, Parts &parts, Rendering rendering= 0)
{
	target.render(parts, rendering);
}

namespace std {
	template <> struct hash <Target>
	{
		size_t operator()(const Target &target) const {
			return hash <string> ()(target.get_text());
		}
	};
}

class Name
/* The possibly parametrized name of a file or transient.  A name has
 * N >= 0 parameters.  When N > 0, the name is parametrized, otherwise
 * it is unparametrized.  A name consists of N+1 static text elements
 * (in the variable TEXTS) and N parameters (in PARAMETERS), which are
 * interleaved.  For instance when N = 2, the name is given by
 *
 * 	texts[0] parameters[0] texts[1] parameters[1] texts[2].
 *
 * Names can be valid or invalid.  A name is valid when all internal
 * texts (between two parameters) are non-empty, and, if N = 0, the
 * single text is non-empty.  A name is empty if N = 0 and the single
 * text is empty (empty names are invalid).  */
{
public:
	Name(string name_):  texts({name_})  {  }
	/* A name with zero parameters */

	Name():  texts({""})  {  }
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

	const vector <string> &get_texts() const {
		return texts;
	}

	const vector <string> &get_parameters() const {
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

	string instantiate(const map <string, string> &mapping) const;
	/* Instantiate the name with the given mapping.  The name may be
	 * empty, resulting in an empty string.  */

	const string &unparametrized() const
	/* Return the name as a string, assuming it is unparametrized.
	 * The name must be unparametrized.  */
	{
		assert(get_n() == 0);
		return texts[0];
	}

	bool match(string name, map <string, string> &mapping,
		   vector <size_t> &anchoring, int &priority) const;
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

	static bool anchoring_dominates(vector <size_t> &anchoring_a,
					vector <size_t> &anchoring_b,
					int priority_a, int priority_b);
	/* Whether anchoring A dominates anchoring B.  The anchorings do
	 * not need to have the same number of parameters.  */

private:
	vector <string> texts; /* Length = N + 1 */
	vector <string> parameters; /* Length = N */
};

void render(const Name &name, Parts &parts, Rendering rendering= 0)
{
	name.render(parts, rendering);
}

class Param_Target
/* A parametrized name for which it is saved what type it represents.
 * Non-dynamic.  */
{
public:
	Flags flags;  /* Only file/transient target info */
	Name name;

	Param_Target(Flags flags_,
		     const Name &name_)
		:  flags(flags_),
		   name(name_)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Param_Target(Target target)
	/* Unparametrized target.   The passed TARGET must be non-dynamic. */
		:  flags(target.get_front_word_nondynamic() & F_TARGET_TRANSIENT),
		   name(target.get_name_nondynamic())
	{
		assert(! target.is_dynamic());
	}

	Target instantiate(const map <string, string> &mapping) const {
		return Target(flags, name.instantiate(mapping));
	}

//	string format_err() const;

	Target unparametrized() const
	/* The corresponding unparametrized target.  This target must
	 * have zero parameters.  */
	{
		return Target(flags, name.unparametrized());
	}

	bool operator== (const Param_Target &that) const {
		return this->flags == that.flags &&
			this->name == that.name;
	}

	bool operator!= (const Param_Target &that) const {
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

	vector <Place> places;
	/* Length = N (number of parameters).
	 * The places of the individual parameters.  */

	Place_Name()
		/* Empty parametrized name, and empty place */
		:  Name(),
		   place()
	{ }

	Place_Name(string name)
		/* Unparametrized, with empty place */
		:  Name(name)
	{
		/* PLACES remains empty */
	}

	Place_Name(string name, const Place &_place)
		/* Unparametrized, with explicit place */
		:  Name(name), place(_place)
	{
		assert(! place.empty());
	}

	const vector <Place> &get_places() const {
		return places;
	}

	void append_parameter(string parameter,
			      const Place &place_parameter)
	/* Append the given PARAMETER and an empty text */
	{
		Name::append_parameter(parameter);
		places.push_back(place_parameter);
	}

	shared_ptr <Place_Name> instantiate(const map <string, string> &mapping) const
	/* In the returned object, the PLACES vector is empty */
	{
		string name= Name::instantiate(mapping);
		return make_shared <Place_Name> (name, place);
	}
};

void show(Parts &, const Place_Name &place_name); 

class Place_Param_Target
/* A target that is parametrized and contains places.  Non-dynamic. */
{
public:
	Flags flags;  /* Only F_TARGET_TRANSIENT is used */
	Place_Name place_name;

	Place place;
	/* The place of the target as a whole.  The PLACE_NAME
	 * variable additionally contains a place for the name itself,
	 * as well as for individual parameters.  */

	Place_Param_Target(Flags flags_,
			   const Place_Name &place_name_)
		:  flags(flags_), place_name(place_name_), place(place_name_.place)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Place_Param_Target(Flags flags_,
			   const Place_Name &place_name_,
			   const Place &place_)
		:  flags(flags_), place_name(place_name_), place(place_)
	{
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0);
	}

	Place_Param_Target(const Place_Param_Target &that)
		:  flags(that.flags),
		   place_name(that.place_name),
		   place(that.place)  {  }

	bool operator==(const Place_Param_Target &that) const
	/* Compares only the content, not the place. */
	{
		return this->flags == that.flags &&
			this->place_name == that.place_name;
	}

	void render(Parts &, Rendering) const;

	shared_ptr <Place_Param_Target>
	instantiate(const map <string, string> &mapping) const {
		return make_shared <Place_Param_Target>
			(flags, *place_name.instantiate(mapping), place);
	}

	Target unparametrized() const {
		return Target(flags, place_name.unparametrized());
	}

	Param_Target get_param_target() const {
		return Param_Target(flags, place_name);
	}

	void canonicalize();  /* In-place */

	static shared_ptr <Place_Param_Target> clone
	(shared_ptr <const Place_Param_Target> place_param_target) {
		shared_ptr <Place_Param_Target> ret= make_shared <Place_Param_Target>
			(place_param_target->flags,
			 place_param_target->place_name,
			 place_param_target->place);
		return ret;
	}
};

void render(const Place_Param_Target &place_param_target,
	    Parts &parts, Rendering rendering= 0)
{
	return place_param_target.render(parts, rendering);
}


shared_ptr <const Place_Param_Target> canonicalize(shared_ptr <const Place_Param_Target> );

#endif /* ! TARGET_HH */
