#ifndef DEP_HH
#define DEP_HH

/*
 * Data types for representing dependencies.  Dependencies are the
 * central data structures in Stu, as all dependencies in the syntax of
 * a Stu script get mapped to Dep objects.  
 *
 * Dependencies are polymorphous objects, and all dependencies derive
 * from the class Dep, and are used via shared_ptr<>, except in
 * cases where access is read-only.  This is necessary in cases where a
 * member function has to access its own THIS pointer, because we can't
 * put THIS into a shared pointer. 
 *
 * All dependency classes allow parametrized targets.  
 */

/*
 * A dependency can be normalized or not.  A dependency is normalized if it
 * is one of:  
 *    - a plain dependency (file or transient);
 *    - a dynamic dependency containing a normalized dependency; 
 *    - a concatenated dependency of only normalized plain and dynamic dependencies.
 * In particular, compound dependencies are never normalized; 
 * they do not appear at all in normalized dependencies. 
 * Also, concatenated dependencies never contain other concatenated dependencies
 * directly -- such constructs are always "flattened" in a normalized
 * dependency. 
 */

/*
 * A plain dependency is a file or a transient. 
 *
 * A dependency is simple when is does not involve concatenation or
 * compound dependencies, i.e., when it is a possible multiply dynamic
 * dependency of a plain dependency.
 *
 * A dependency that is not simple is complex.  I.e., a complex
 * dependency involves concatenation and/or compound dependencies. 
 */

#include <map>

#ifndef NDEBUG
#	include <bitset>
#endif

#include "error.hh"
#include "target.hh"
#include "flags.hh"

template <typename T, typename U>
shared_ptr <const T> to(shared_ptr <const U> d)
{
	return dynamic_pointer_cast <const T> (d); 
}

template <typename T, typename U>
shared_ptr <const T> to(shared_ptr <U> d)
{
	return dynamic_pointer_cast <const T> (d); 
}

class Dep
/* 
 * The abstract base class for all dependencies.  
 *
 * The flags only represent immediate flags.  Compound dependencies for
 * instance may contain additional inner flags. 
 *
 * Objects of type Dep and subclasses are always handled through
 * shared_ptr<>.  All objects may have many persistent pointers to it,
 * so they are considered final, i.e., immutable, except if we just
 * created the object in which case we know that it is not shared.
 * Therefore, we always use shared_ptr <const ...>, except when we just
 * created the dependency.  All dependencies are created via
 * make_shared<>. 
 *
 * The use of shared_ptr<> also means that certain functions cannot be
 * member functions but must be static functions instead:  clone(),
 * normalize(), etc.  This is because we cannot use a construct like
 * shared_ptr <Dep> (this), which is erroneous (the object would
 * be released twice, etc.).  As a result, we replace THIS by an
 * argument of type shared_ptr<>.  [Note:  there is also
 * std::enable_shared_from_this as a possibility.]
 *
 * The constructors of Dep and derived classes do not set the TOP and
 * INDEX fields.  These are set manually when needed. 
 */ 
{
public:
	Flags flags;

	Place places[C_PLACED]; 
	/* For each transitive flag that is set, the place.  An empty
	 * place if a flag is not set  */

	shared_ptr <const Dep> top; 
	/* Additional place used for constructing traces.  Most of the
	 * properties (such as extra flags) are ignored.  */

	ssize_t index; 
	/* Used by concatenated executions; the index of the dependency
	 * within the array of concatenation.  -1 when not used. */

	Dep()
		:  flags(0),
		   index(-1)
	{  }

	Dep(Flags flags_) 
		:  flags(flags_),
		   index(-1)
	{  }

	Dep(Flags flags_, const Place places_[C_PLACED])
		:  flags(flags_),
		   index(-1)
	{
		assert(places != places_);
		for (unsigned i= 0;  i < C_PLACED;  ++i)
			places[i]= places_[i]; 
	}

	Dep(const Dep &that)
		:  flags(that.flags),
		   top(that.top),
		   index(that.index)
	{
		assert(places != that.places);
		for (unsigned i= 0;  i < C_PLACED;  ++i)
			places[i]= that.places[i]; 
	}

	virtual ~Dep() = default; 

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
	/* Add the flags from DEP.  Also copy over the
	 * corresponding places.  If a place is already given in THIS,
	 * only copy a place over if OVERWRITE_PLACES is set.  */

	/* The check function checks the internal consistency of a
	 * Dep object.  This is purely an assertion, and not a
	 * programmatic check.  It is possible for Dep objects to
	 * be temporarily inconsistent while they are changed --
	 * therefore, consistency is not enforced by the accessor
	 * functions, but only by this function.  */
#ifndef NDEBUG
	void check() const;
#else
	void check() const {  }
#endif		

	virtual shared_ptr <const Dep> instantiate(const map <string, string> &mapping) const= 0;
	virtual bool is_unparametrized() const= 0; 

	virtual const Place &get_place() const= 0;
	/* Where the dependency as a whole is declared */ 

	virtual string format(Style, bool &quotes) const= 0; 
	virtual string format_err() const= 0; 
	virtual string format_out() const= 0; 
	virtual string format_src() const= 0; 

	virtual Target get_target() const= 0;
	/* Get the corresponding Target object.  Only called for
	 * non-compound and non-parametrized dependencies.  */

	virtual bool is_normalized() const= 0;

	static void normalize(shared_ptr <const Dep> dep,
			      vector <shared_ptr <const Dep> > &deps,
			      int &error);
	/* Split DEP into multiple DEPS that are each
	 * normalized.  The resulting dependencies are appended to
	 * DEPS, which does not have to be empty on entering the
	 * function.  
	 * On errors, a message is printed, bits are set in ERROR, and
	 * if not in keep-going mode, the function returns immediately. 
	 */

	static shared_ptr <Dep> clone(shared_ptr <const Dep> dep);
	/* A shallow clone */

	static shared_ptr <const Dep> strip_dynamic(shared_ptr <const Dep> d);
	/* Strip dynamic dependencies from the given dependency.
	 * Perform recursively:  If D is a dynamic dependency, return
	 * its contained dependency, otherwise return D.  Thus, never
	 * return null.  */
};

class Plain_Dep
/* 
 * A dependency denoting an individual target name, which can be a file
 * or a transient.  
 *
 * When the target is a transient, the dependency flags have the
 * F_TARGET_TRANSIENT bit set, which is redundant, because that
 * information is also contained in PLACE_PARAM_TARGET.  No other Dep
 * type has the F_TARGET_TRANSIENT flag set.
 */
	:  public Dep
{
public:

	Place_Param_Target place_param_target; 
	/* The target of the dependency.  Has its own place, which may
	 * differ from the dependency's place, e.g. in '@all'.  Is
	 * non-dynamic.  */

	Place place;
	/* The place where the dependency is declared */ 

	string variable_name;
	/* With F_VARIABLE:  the name of the variable.
	 * Otherwise:  empty.  */

	explicit Plain_Dep(const Place_Param_Target &place_param_target_)
		:  Dep(place_param_target_.flags),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{
		check(); 
	}
	
	Plain_Dep(Flags flags_,
		  const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Dep(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Plain_Dep(Flags flags_,
		  const Place places_[C_PLACED],
		  const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Dep(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Plain_Dep(Flags flags_,
		  const Place_Param_Target &place_param_target_,
		  const Place &place_,
		  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dep(flags_),
		   place_param_target(place_param_target_),
		   place(place_),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Plain_Dep(Flags flags_,
		  const Place places_[C_PLACED],
		  const Place_Param_Target &place_param_target_,
		  const Place &place_,
		  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dep(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Plain_Dep(Flags flags_,
		  const Place places_[C_PLACED],
		  const Place_Param_Target &place_param_target_,
		  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dep(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Plain_Dep(const Plain_Dep &plain_dep)
		:  Dep(plain_dep),
		   place_param_target(plain_dep.place_param_target),
		   place(plain_dep.place),
		   variable_name(plain_dep.variable_name)
	{  }

	const Place &get_place() const {
		return place; 
	}

	virtual shared_ptr <const Dep> instantiate(const map <string, string> &mapping) const;

	bool is_unparametrized() const {
		return place_param_target.place_name.get_n() == 0; 
	}

	virtual string format(Style style, bool &quotes) const;
	virtual string format_err() const;
	virtual string format_out() const;
	virtual string format_src() const;

	virtual bool is_normalized() const { return true;  }

	virtual Target get_target() const;
	/* Does not preserve the F_VARIABLE bit */ 
};

class Dynamic_Dep
/*
 * The Dep::flags field has the F_TARGET_DYNAMIC set. 
 */
	:  public Dep
{
public:

	shared_ptr <const Dep> dep;
	/* The contained dependency.  Non-null. */ 

	Dynamic_Dep(shared_ptr <const Dep> dep_)
		/* Set the contained dependency.  NOT a copy constructor. */
		:  Dep(F_TARGET_DYNAMIC),
		   dep(dep_)
	{
		assert(dep_ != nullptr); 
	}

	Dynamic_Dep(Flags flags_,
		    shared_ptr <const Dep> dep_)
		:  Dep(flags_ | F_TARGET_DYNAMIC), 
		   dep(dep_)
	{
		assert((flags & F_VARIABLE) == 0); 
		assert(dep_ != nullptr); 
	}

	Dynamic_Dep(Flags flags_,
		    const Place places_[C_PLACED],
		    shared_ptr <const Dep> dep_)
		:  Dep(flags_ | F_TARGET_DYNAMIC, places_),
		   dep(dep_)
	{
		assert((flags & F_VARIABLE) == 0); /* Variables cannot be dynamic */
		assert(dep_ != nullptr); 
	}

	virtual shared_ptr <const Dep>  instantiate(const map <string, string> &mapping) const;
	bool is_unparametrized() const {  return dep->is_unparametrized();  }

	const Place &get_place() const 
	/* In error message pointing to dynamic dependency such as
	 * '[B]', it is more useful to the user to point to the 'B' than
	 * to the '['.  */
	{
		return dep->get_place(); 
	}

	virtual string format(Style, bool &quotes) const;
	virtual string format_err() const;
	virtual string format_out() const;
	virtual string format_src() const;

	virtual bool is_normalized() const {
		return dep->is_normalized(); 
	}

	virtual Target get_target() const;

	unsigned get_depth() const 
	/* The depth of the dependency, i.e., how many dynamic
	 * dependencies are stacked in a row.  */
	{
		if (to <Dynamic_Dep> (dep)) {
			return 1 + to <Dynamic_Dep> (dep)->get_depth(); 
		} else {
			return 1; 
		}
	}
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
	:  public Dep
{
public:

	vector <shared_ptr <const Dep> > deps;
	/* The dependencies for each part.  No entry is null.  
	 * May be empty in code, which is something
	 * that is not allowed in Stu code.  Otherwise, there are at
	 * least two elements  */

	Concat_Dep()
	/* An empty concatenation, i.e., a concatenation of zero dependencies */ 
	{  }

	Concat_Dep(Flags flags_, const Place places_[C_PLACED])
		/* The list of dependencies is empty */ 
		:  Dep(flags_, places_)
	{  }

	/* Append a dependency to the list */
	void push_back(shared_ptr <const Dep> dep)
	{
		deps.push_back(dep); 
	}

	virtual shared_ptr <const Dep> instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const;

	virtual string format(Style, bool &quotes) const; 
	virtual string format_err() const; 
	virtual string format_out() const; 
	virtual string format_src() const; 

	virtual bool is_normalized() const; 

	virtual Target get_target() const;

	static shared_ptr <const Dep> concat(shared_ptr <const Dep> a,
					     shared_ptr <const Dep> b,
					     int &error); 
	/* Concatenate two dependencies to a single dependency.  On
	 * error, a message is printed, bits are set in ERROR, and null
	 * is returned.  Only plain and dynamic dependencies can be passed.  */

	static shared_ptr <const Plain_Dep> concat_plain(shared_ptr <const Plain_Dep> a,
							 shared_ptr <const Plain_Dep> b);
	static shared_ptr <const Concat_Dep> concat_complex(shared_ptr <const Dep> a,
							    shared_ptr <const Dep> b);

	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     vector <shared_ptr <const Dep> > &deps,
				     int &error); 
	/* Normalize this object's dependencies into a list of individual
	 * dependencies.  The generated dependencies are appended to
	 * DEPS which does not need to be empty on entry into
	 * this function.  
	 * On errors, a message is printed, bits are set in ERROR, and
	 * if not in keep-going mode, the function returns immediately. 
	 */

	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     vector <shared_ptr <const Dep> > &deps,
				     size_t start_index,
				     int &error);
	/* Helper function.  Write result into DEPS,
	 * concatenating all starting at the given index.  
	 * On errors, a message is printed, bits are set in ERROR, and
	 * if not in keep-going mode, the function returns immediately. 
	 */
};

class Compound_Dep
/* 
 * A list of dependencies that act as a unit, corresponding
 * syntactically to a list of dependencies in parentheses.  
 *
 * In terms of Stu source code, a compound dependency corresponds to
 *
 *         (X Y Z ...)
 *
 * Compound dependencies are themselves never normalized.  Within
 * normalized dependencies, they appear only as immediate children of
 * concatenated dependencies.  Otherwise, they also appear after parsing
 * to denote syntactic groups of dependencies. 
 */
	:  public Dep
{
public:

	Place place; 
	/* The place of the compound ; usually the opening parenthesis
	 * or brace.  May be empty to denote no place, in particular if
	 * this is a "logical" compound dependency not coming from a
	 * parenthesised expression.  */

	vector <shared_ptr <const Dep> > deps;
	/* The contained dependencies, in given order */ 

	Compound_Dep(const Place &place_) 
		/* Empty, with zero dependencies */
		:  place(place_)
	{  }
	
	Compound_Dep(Flags flags_, const Place places_[C_PLACED], const Place &place_)
		:  Dep(flags_, places_),
		   place(place_)
	{
		/* The list of dependencies is empty */ 
	}

	Compound_Dep(vector <shared_ptr <const Dep> > &&deps_, 
		     const Place &place_)
		:  place(place_),
		   deps(deps_)
	{  }

	void push_back(shared_ptr <const Dep> dep)
	{
		deps.push_back(dep); 
	}

	virtual shared_ptr <const Dep> instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const
	{
		return place; 
	}

	virtual string format(Style, bool &quotes) const; 
	virtual string format_err() const; 
	virtual string format_out() const; 
	virtual string format_src() const; 

	virtual bool is_normalized() const {  return false;  }
	/* A compound dependency is never normalized */

	virtual Target get_target() const {  assert(false);  return Target();  }
};

class Root_Dep
/*
 * Dependency to denote the root object of the dependency tree.  There
 * is just one possible value of this, and it is never shown to the
 * user, but used internally with the root execution object. 
 */
	:  public Dep
{
public:
	virtual shared_ptr <const Dep> instantiate(const map <string, string> &) const {
		return shared_ptr <const Dep> (make_shared <Root_Dep> ()); 
	}
	virtual bool is_unparametrized() const {  return false;  }
	virtual const Place &get_place() const {  return Place::place_empty;  }
	virtual string format(Style, bool &) const {  return "ROOT";  }
	virtual string format_err() const {  assert(false);  return "";  }
	virtual string format_out() const {  return "ROOT";  }
	virtual string format_src() const {  return "ROOT";  }
	virtual Target get_target() const {  return Target();  }
	virtual bool is_normalized() const {  return true;  }
};

void Dep::normalize(shared_ptr <const Dep> dep,
		    vector <shared_ptr <const Dep> > &deps,
		    int &error)
{
	if (to <Plain_Dep> (dep)) {
		deps.push_back(dep);
	} else if (shared_ptr <const Dynamic_Dep> dynamic_dep= to <Dynamic_Dep> (dep)) {
		vector <shared_ptr <const Dep> > deps_child;
		normalize(dynamic_dep->dep, deps_child, error);
		if (error && ! option_keep_going)
			return;
		for (auto &d:  deps_child) {
			shared_ptr <Dep> dep_new= 
				make_shared <Dynamic_Dep> 
				(dynamic_dep->flags, dynamic_dep->places, d);
			if (dynamic_dep->index >= 0)
				dep_new->index= dynamic_dep->index;
			dep_new->top= dynamic_dep->top;
			deps.push_back(dep_new); 
		}
	} else if (shared_ptr <const Compound_Dep> compound_dep= to <Compound_Dep> (dep)) {
		for (auto &d:  compound_dep->deps) {
			shared_ptr <Dep> dd= Dep::clone(d); 
			dd->add_flags(compound_dep, false);  
			if (compound_dep->index >= 0)
				dd->index= compound_dep->index;
			dd->top= compound_dep->top;
			normalize(dd, deps, error); 
			if (error && ! option_keep_going)
				return; 
		}
	} else if (auto concat_dep= to <Concat_Dep> (dep)) {
		Concat_Dep::normalize_concat(concat_dep, deps, error);
		if (error && ! option_keep_going)
			return; 
	} else {
		assert(false);
	}
}

shared_ptr <Dep> Dep::clone(shared_ptr <const Dep> dep)
{
	assert(dep); 

	if (to <Plain_Dep> (dep)) {
		return make_shared <Plain_Dep> (* to <Plain_Dep> (dep)); 
	} else if (to <Dynamic_Dep> (dep)) {
		return make_shared <Dynamic_Dep> (* to <Dynamic_Dep> (dep)); 
	} else if (to <Compound_Dep> (dep)) {
		return make_shared <Compound_Dep> (* to <Compound_Dep> (dep)); 
	} else if (to <Concat_Dep> (dep)) {
		return make_shared <Concat_Dep> (* to <Concat_Dep> (dep)); 
	} else if (to <Root_Dep> (dep)) {
		return make_shared <Root_Dep> (* to <Root_Dep> (dep)); 
	} else {
		/* Bug:  Unhandled dependency type */ 
		assert(false);
		return nullptr; 
	}
}

void Dep::add_flags(shared_ptr <const Dep> dep, 
		    bool overwrite_places)
{
	for (unsigned i= 0;  i < C_PLACED;  ++i) {
		if (dep->flags & (1 << i)) {
			if (overwrite_places || ! (this->flags & (1 << i))) {
				this->set_place_flag(i, dep->get_place_flag(i)); 
			}
		}
	}
	this->flags |= dep->flags; 
}

shared_ptr <const Dep> Dep::strip_dynamic(shared_ptr <const Dep> d)
{
	assert(d != nullptr); 
	while (to <Dynamic_Dep> (d)) {
		d= to <Dynamic_Dep> (d)->dep;
	}
	assert(d != nullptr); 
	return d;
}

#ifndef NDEBUG
void Dep::check() const
{
	assert(top.get() != this); 

	for (unsigned i= 0;  i < C_PLACED;  ++i) {
		assert(((flags & (1 << i)) == 0) == get_place_flag(i).empty()); 
	}

	if (auto plain_this= dynamic_cast <const Plain_Dep *> (this)) {
		/* The F_TARGET_TRANSIENT flag is always set in the
		 * dependency flags, even though that is redundant.  */
		assert((plain_this->flags & F_TARGET_TRANSIENT) == (plain_this->place_param_target.flags)); 

		if (plain_this->variable_name != "") {
			assert((plain_this->place_param_target.flags & F_TARGET_TRANSIENT) == 0); 
			assert(plain_this->flags & F_VARIABLE); 
		}
	}

	if (auto dynamic_this= dynamic_cast <const Dynamic_Dep *> (this)) {
		assert(flags & F_TARGET_DYNAMIC); 
		dynamic_this->dep->check(); 
	} else {
		assert(!(flags & F_TARGET_DYNAMIC)); 
	}

	if (auto concat_this= dynamic_cast <const Concat_Dep *> (this)) {
		assert(concat_this->deps.size() >= 2); 
		for (auto i:  concat_this->deps) {
			assert(i); 
		}
	}
	
	assert(index >= -1); 
}
#endif


Target Plain_Dep::get_target() const
{
	Target ret= place_param_target.unparametrized(); 
	ret.get_front_word_nondynamic() |= (word_t)(flags & F_TARGET_BYTE);
	return ret; 
}

string Plain_Dep::format(Style style, bool &quotes) const 
{
	string f;
	if (!(style & S_NOFLAGS)) {
		f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT)); 
		if (f != "") {
			style |= S_MARKERS;
			f += ' '; 
		}
	}
	bool detached= (flags & F_VARIABLE) || (flags & F_TARGET_TRANSIENT); 
	bool quotes_inner= detached ? false : quotes;
	string t= place_param_target.format(style & ~S_COLOR_WORD, quotes_inner);
	if (detached)
		quotes= false;
	else 
		quotes= quotes_inner; 
	bool quotes_print=  detached && quotes_inner;
	string ret= fmt("%s%s%s%s%s%s",
			f,
			flags & F_VARIABLE ? "$[" : "",
			quotes_print ? "'" : "",
			t,
			quotes_print ? "'" : "",
			flags & F_VARIABLE ? "]" : "");
	if (style & S_COLOR_WORD) {
		ret= fmt("%s%s%s", Color::word, ret, Color::end); 
	}
	return ret;
}

string Plain_Dep::format_out() const 
{
	bool quotes= false;
	string ret= format(S_NOFLAGS | S_OUT, quotes);
	if (quotes)
		ret= fmt("\'%s\'", ret); 
	return ret; 
}

string Plain_Dep::format_src() const 
{
	bool quotes= false;
	string ret= format(S_SRC, quotes);
	if (quotes)
		ret= fmt("\'%s\'", ret); 
	return ret; 
}

string Plain_Dep::format_err() const
{
	bool quotes= Color::quotes;
	string ret= format(S_NOFLAGS | S_ERR | S_COLOR_WORD, quotes);
	if (quotes)
		ret= fmt("\'%s\'", ret); 
	return ret; 
}

Target Dynamic_Dep::get_target() const
{
	string text;
	const Dep *d= this; 
	while (dynamic_cast <const Dynamic_Dep *> (d)) {
		Flags f= F_TARGET_DYNAMIC; 
		assert(d->flags & F_TARGET_DYNAMIC); 
		f |= d->flags & F_TARGET_BYTE; 
		text += Target::string_from_word(f); 
		d= dynamic_cast <const Dynamic_Dep *> (d)->dep.get(); 
	}
	assert(dynamic_cast <const Plain_Dep *> (d)); 
	const Plain_Dep *sin= dynamic_cast <const Plain_Dep *> (d); 
	assert(!(sin->flags & F_TARGET_DYNAMIC)); 
	Flags f= sin->flags & F_TARGET_BYTE;
	text += Target::string_from_word(f); 
	text += sin->place_param_target.unparametrized().get_name_nondynamic(); 
	
	return Target(text); 
}

string Dynamic_Dep::format(Style style, bool &quotes) const 
{
	quotes= false;
	bool quotes_inner= false;
	string ret;
	if (! (style & S_NOFLAGS)) {
		string text_flags= flags_format(flags & ~F_TARGET_DYNAMIC);
		if (text_flags != "")
			text_flags += ' '; 
		ret += text_flags; 
	}
	string text= dep->format((style | S_MARKERS) & ~S_COLOR_WORD, quotes_inner);
	ret += fmt("[%s%s%s]",
		   quotes_inner ? "'" : "",
		   text,
		   quotes_inner ? "'" : "");
	if (style & S_COLOR_WORD) {
		ret= fmt("%s%s%s", Color::word, ret, Color::end); 
	}
	return ret; 
}

string Dynamic_Dep::format_out() const
{
	bool quotes= false;
	return format(S_NOFLAGS | S_OUT, quotes);
}

string Dynamic_Dep::format_src() const
{
	bool quotes= false;
	return format(S_SRC, quotes);
}

string Dynamic_Dep::format_err() const
{
	bool quotes= false;
	return format(S_NOFLAGS | S_ERR | S_COLOR_WORD, quotes);
}

shared_ptr <const Dep> Dynamic_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Dynamic_Dep> ret= make_shared <Dynamic_Dep> (flags, places, dep->instantiate(mapping));
	ret->index= index;
	ret->top= top; 
	return ret;
}

shared_ptr <const Dep> Plain_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dep> ret= make_shared <Plain_Dep> (flags, places, *ret_target, place, variable_name);
	ret->index= index;
	ret->top= top; 

	assert(ret_target->place_name.get_n() == 0); 

	string this_name= ret_target->place_name.unparametrized(); 
	if ((flags & F_VARIABLE) && this_name.find('=') != string::npos) {
		assert((ret_target->flags & F_TARGET_TRANSIENT) == 0); 
		place << fmt("dynamic variable %s must not be instantiated with parameter value that contains %s", 
			     dynamic_variable_format_err(this_name),
			     char_format_err('='));
		throw ERROR_LOGICAL; 
	}

	return ret;
}

shared_ptr <const Dep> 
Compound_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Compound_Dep> ret= make_shared <Compound_Dep> (flags, places, place);
	ret->index= index;
	ret->top= top; 

	for (const shared_ptr <const Dep> &d:  deps) {
		ret->push_back(d->instantiate(mapping));
	}
	
	return ret; 
}

bool Compound_Dep::is_unparametrized() const
/* A compound dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d:  deps) {
		if (! d->is_unparametrized())
			return false;
	}

	return true;
}

string Compound_Dep::format(Style style, bool &) const
/* Ignore QUOTES, as everything inside the parentheses will not need
 * it.  */  
{
	string ret;
	bool quotes= false;
	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += " ";
		ret += d->format(style, quotes); 
	}
	if (deps.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dep::format_err() const
{
	bool quotes= Color::quotes;
	string ret= format(S_ERR | S_NOFLAGS | S_COLOR_WORD, quotes); 
	if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

string Compound_Dep::format_out() const
{
	bool quotes= Color::quotes;
	string ret= format(S_OUT | S_NOFLAGS, quotes);
		if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

string Compound_Dep::format_src() const
{
	bool quotes= Color::quotes;
	string ret= format(S_SRC, quotes); 
	if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

shared_ptr <const Dep> Concat_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Concat_Dep> ret= make_shared <Concat_Dep> (flags, places);
	ret->index= index;
	ret->top= top; 

	for (const shared_ptr <const Dep> &d:  deps) {
		ret->push_back(d->instantiate(mapping)); 
	}

	return ret; 
}

bool Concat_Dep::is_unparametrized() const
/* A concatenated dependency is parametrized when any of its contained 
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d:  deps) {
		if (! d->is_unparametrized())
			return false;
	}

	return true;
}

const Place &Concat_Dep::get_place() const
/* Return the place of the first dependency, or an empty place */
{
	if (deps.empty())
		return Place::place_empty;

	return deps.front()->get_place(); 
}

string Concat_Dep::format(Style style, bool &quotes) const
/* We only need quotes when *all* components need quotes */	
{
	assert(bitset <sizeof(Style)> (style & S_CHANNEL).count() <= 1); 
	string ret;
	if (!(style & S_NOFLAGS)) {
		string f= flags_format(flags); 
		if (f != "") {
			style |= S_MARKERS;
			f += ' '; 
		}
		ret += f;
	}
	bool quotes_ret= true; 
	for (const shared_ptr <const Dep> &d:  deps) {
		bool quotes_d= quotes;
		ret += d->format(style, quotes_d); 
		if (! quotes_d)
			quotes_ret= false; 
	}
	quotes= quotes_ret; 
	return ret; 
}

string Concat_Dep::format_err() const
{
	bool quotes= Color::quotes;
	string ret= format(S_ERR | S_NOFLAGS | S_COLOR_WORD, quotes); 
	if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

string Concat_Dep::format_out() const
{
	bool quotes= Color::quotes;
	string ret= format(S_OUT | S_NOFLAGS, quotes); 
	if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

string Concat_Dep::format_src() const
{
	bool quotes= Color::quotes;
	string ret= format(S_SRC, quotes); 
	if (quotes)
		ret= '\'' + ret + '\''; 
	return ret; 
}

bool Concat_Dep::is_normalized() const
{
	for (auto &i:  deps) {
		if (to <const Concat_Dep> (i)) 
			return false; 
		if (! i->is_normalized())
			return false;
	}
	return true;
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep,
				  vector <shared_ptr <const Dep> > &deps_,
				  int &error) 
{
	size_t k_init= deps_.size(); 

	normalize_concat(dep, deps_, 0, error); 

	if (error && ! option_keep_going)
		return;

	/* Add attributes from DEP */ 	

	if (dep->flags || dep->index >= 0 || dep->top) {
		for (size_t k= k_init;  k < deps_.size();  ++k) {
			shared_ptr <Dep> d_new= Dep::clone(deps_[k]); 
			/* The innermost flag is kept */
			d_new->add_flags(dep, false); 
			if (dep->index >= 0)
				d_new->index= dep->index;
			d_new->top= dep->top; 
			deps_[k]= d_new;
		}
	}
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep, 
				  vector <shared_ptr <const Dep> > &deps_,
				  size_t start_index,
				  int &error) 
{
	assert(start_index < dep->deps.size()); 

	if (start_index + 1 == dep->deps.size()) {
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d:  compound_dd->deps) {
				normalize(d, deps_, error); 
				if (error && ! option_keep_going)
					return;
			}
		} else if (to <Plain_Dep> (dd)) {
			deps_.push_back(dd); 
		} else if (auto concat_dd= to <Concat_Dep> (dd)) {
			normalize_concat(concat_dd, deps_, error);
			if (error && ! option_keep_going)
				return;
		} else if (to <Dynamic_Dep> (dd)) {
			normalize(dd, deps_, error); 
			if (error && ! option_keep_going)
				return;
		} else {
			assert(false); 
		}
	} else {
		vector <shared_ptr <const Dep> > vec1, vec2;
		normalize_concat(dep, vec2, start_index + 1, error); 
		if (error && ! option_keep_going)
			return; 
		shared_ptr <const Dep> dd= dep->deps.at(start_index); 
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d:  compound_dd->deps) {
				normalize(d, vec1, error); 
				if (error && ! option_keep_going)
					return; 
			}
		} else if (to <Plain_Dep> (dd)) {
			vec1.push_back(dd); 
		} else if (to <Dynamic_Dep> (dd)) {
			normalize(dd, vec1, error); 
			if (error && ! option_keep_going)
				return;
		} else if (auto concat_dd= to <Concat_Dep> (dd)) {
			normalize_concat(concat_dd, vec1, error); 
			if (error && ! option_keep_going)
				return; 
		} else {
			assert(false); 
		}

		for (const auto &d1:  vec1) {
			for (const auto &d2:  vec2) {
				shared_ptr <const Dep> d= concat(d1, d2, error);
				if (error && ! option_keep_going) 
					return; 
				if (d) 
					deps_.push_back(d); 
			}
		}
	}
}

Target Concat_Dep::get_target() const
{
	/* Dep::get_target() is not used for complex dependencies */
	assert(false);
	return Target(); 
}

shared_ptr <const Dep> Concat_Dep::concat(shared_ptr <const Dep> a,
					  shared_ptr <const Dep> b,
					  int &error)
{
	assert(a);
	assert(b); 

	/*
	 * Check for invalid combinations
	 */

	if (a->flags & F_INPUT) {
		/* It would in principle be possible to allow
		 * concatenations in which the left component has an
		 * input redirection, but the current data structures do
		 * not allow that, and therefore we make that invalid.  */
		a->get_place() << fmt("%s cannot have input redirection using %s",
				      a->format_err(),
				      char_format_err('<')); 
		b->get_place() << fmt("because %s is concatenated to it",
				      b->format_err()); 
		error |= ERROR_LOGICAL;
		return nullptr; 
	}

	if (b->flags & F_INPUT) {
		/* We don't save the place for the '<', so we cannot
		 * have "using '<'" on an extra line.  */
		b->get_place() << fmt("%s cannot have input redirection using %s", 
				      b->format_err(),
				      char_format_err('<')); 
		a->get_place() << fmt("in concatenation to %s", a->format_err()); 
		error |= ERROR_LOGICAL;
		return nullptr; 
	}

	if (b->flags & F_PLACED) {
		static_assert(C_PLACED == 3, "Expected C_PLACED == 3"); 
		unsigned i_flag= 
			b->flags & F_PERSISTENT ? I_PERSISTENT :
			b->flags & F_OPTIONAL   ? I_OPTIONAL   :
			b->flags & F_TRIVIAL    ? I_TRIVIAL    : 
			C_ALL;
		assert(i_flag != C_ALL); 
		b->get_place() << fmt("%s cannot be declared as %s",
				      b->format_err(), FLAGS_PHRASES[i_flag]); 
		b->places[i_flag] << fmt("using %s",
					 name_format_err(frmt("-%c", FLAGS_CHARS[i_flag]))); 
		a->get_place() << fmt("in concatenation to %s", a->format_err()); 
		error |= ERROR_LOGICAL;
		return nullptr; 
	}

	if (b->flags & F_TARGET_TRANSIENT) {
		b->get_place() << fmt("transient target %s is invalid", b->format_err()); 
		a->get_place() << fmt("in concatenation to %s", a->format_err()); 
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (a->flags & F_VARIABLE) {
		a->get_place() << fmt("the variable dependency %s cannot be used", 
				      a->format_err());
		b->get_place() << fmt("in concatenation with %s", 
				      b->format_err());
		error |= ERROR_LOGICAL;
		return nullptr; 
	}

	if (b->flags & F_VARIABLE) {
		b->get_place() << fmt("variable dependency %s is invalid", b->format_err());
		a->get_place() << fmt("in concatenation to %s", a->format_err()); 
		error |= ERROR_LOGICAL; 
		return nullptr;
	}

	if (to <const Plain_Dep> (a) && to <const Plain_Dep> (b))
		return concat_plain(to <const Plain_Dep> (a), to <const Plain_Dep> (b)); 
	else
		return concat_complex(a, b); 
}

shared_ptr <const Plain_Dep> Concat_Dep::concat_plain(shared_ptr <const Plain_Dep> a,
						      shared_ptr <const Plain_Dep> b)
{
	assert(a);
	assert(b);

	/* Parametrized dependencies are instantiated first before they
	 * are concatenated  */
	assert(! a->place_param_target.place_name.is_parametrized());  
	assert(! b->place_param_target.place_name.is_parametrized());  
	
	/*
	 * Combine 
	 */ 

	Flags flags_combined= a->flags | b->flags; 

	Place_Name place_name_combined(a->place_param_target.place_name.unparametrized() +
				       b->place_param_target.place_name.unparametrized(),
				       a->place_param_target.place_name.place); 

	shared_ptr <Plain_Dep> ret= 
		make_shared <Plain_Dep> (flags_combined,
					 a->places,
					 Place_Param_Target(flags_combined & F_TARGET_TRANSIENT,
							    place_name_combined,
							    a->place_param_target.place),
					 a->place, ""); 
	ret->top= a->top;
	if (! ret->top)
		ret->top= b->top; 
	return ret; 
}

shared_ptr <const Concat_Dep> Concat_Dep::concat_complex(shared_ptr <const Dep> a,
							 shared_ptr <const Dep> b)
/* We don't have to make any checks here because any errors will be
 * caught later when the resulting plain dependencies are concatenated.
 * However, checking errors here is faster, since it avoids building
 * dynamic dependencies unnecessarily.  */
{
	assert(! (to <const Plain_Dep> (a) && to <const Plain_Dep> (b))); 

	shared_ptr <Concat_Dep> ret= make_shared <Concat_Dep> (); 

	if (auto concat_a= to <const Concat_Dep> (a)) {
		for (auto d:  concat_a->deps) 
			ret->push_back(d); 
	} else {
		ret->push_back(a);
	}

	if (auto concat_b= to <const Concat_Dep> (b)) {
		for (auto d:  concat_b->deps) 
			ret->push_back(d); 
	} else {
		ret->push_back(b); 
	}

	return ret; 
}

#endif /* ! DEP_HH */
