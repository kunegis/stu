#ifndef DEP_HH
#define DEP_HH

/*
 * Data types for representing dependencies.  Dependencies are the
 * central data structures in Stu, as all dependencies in the syntax of
 * a Stu file get mapped to Dep objects.  
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
 *    - a concatenated dependency, each of whose component is either a
 *      plain dependency, a dynamic dependency, or a compound dependency 
 * 	of plain and dynamic dependencies. 
 * In particular, compound dependencies are never normalized; they only
 * appear immediately within concatenated dependencies.  
 * Normalized dependencies are those used in practice.  A non-normalized
 * dependency can always be reduced to a normalized one. 
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
 * The abstract base class for all dependencies.  Objects of this type
 * are used via shared_ptr<>.
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
 */ 
{
public:

	Flags flags;

	Place places[C_PLACED]; 
	/* For each transitive flag that is set, the place.  An empty
	 * place if a flag is not set  */

	shared_ptr <const Dep> top; 
	/* Additional place.  Most of the properties (such as extra
	 * flags) are ignored.  */

	Dep()
		:  flags(0)
	{  }

	Dep(Flags flags_) 
		:  flags(flags_)
	{  }

	Dep(Flags flags_, const Place places_[C_PLACED])
		:  flags(flags_)
	{
		assert(places != places_);
		for (int i= 0;  i < C_PLACED;  ++i)
			places[i]= places_[i]; 
	}

	virtual ~Dep(); 

	const Place &get_place_flag(int i) const {
		assert(i >= 0 && i < C_PLACED);
		return places[i];
	}

	Place &get_place_flag(int i) {
		assert(i >= 0 && i < C_PLACED);
		return places[i];
	}

	void set_place_flag(int i, const Place &place) {
		assert(i >= 0 && i < C_PLACED);
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
	virtual string format_word() const= 0; 
	virtual string format_out() const= 0; 
	virtual string format_src() const= 0; 

	virtual Target get_target() const= 0;
	/* Get the corresponding Target object.  Only called for
	 * non-compound and non-parametrized dependencies.  */

	virtual bool is_normalized() const= 0;

	static void normalize(shared_ptr <const Dep> dep,
			      vector <shared_ptr <const Dep> > &deps);
	/* Split DEP into multiple DEPS that are each
	 * normalized.  The resulting dependencies are appended to
	 * DEPS, which does not have to be empty on entering the
	 * function.  */

	static shared_ptr <const Dep> normalize_compound(shared_ptr <const Dep> dep);
	/* Return either a normalized version of the given dependency, or a
	 * Compound_Dep containing normalized dependencies */ 

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
 * F_TARGET_TRANSIENT bit set, which is redundant.  No other Dep
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

	shared_ptr <const Dep> instantiate(const map <string, string> &mapping) const;

	bool is_unparametrized() const {
		return place_param_target.place_name.get_n() == 0; 
	}

	virtual string format(Style style, bool &quotes) const {
		string f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT)); 
		if (f != "") {
			style |= S_MARKERS;
			f += ' '; 
		}
		string t= place_param_target.format(style, quotes);
		return fmt("%s%s%s%s%s%s",
			   f,
			   flags & F_VARIABLE ? "$[" : "",
			   quotes ? "'" : "",
			   t,
			   quotes ? "'" : "",
			   flags & F_VARIABLE ? "]" : "");
	}

	virtual string format_word() const;
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

	shared_ptr <const Dep>  instantiate(const map <string, string> &mapping) const
	{
		return make_shared <Dynamic_Dep> (flags, dep->instantiate(mapping));
	}

	bool is_unparametrized() const
	{
		return dep->is_unparametrized(); 
	}

	const Place &get_place() const 
	/* In error message pointing to dynamic dependency such as
	 * '[B]', it is more useful to the user to point to the 'B' than
	 * to the '['.  */
	{
		return dep->get_place(); 
	}

	virtual string format(Style, bool &quotes) const {
		quotes= false;
		bool quotes2= false;
		string s= dep->format(S_MARKERS, quotes2);
		return fmt("[%s%s%s]",
			   quotes2 ? "'" : "",
			   s,
			   quotes2 ? "'" : "");
	}

	virtual string format_word() const {
		bool quotes= false;
		string s= dep->format(S_MARKERS, quotes);
		return fmt("%s[%s%s%s]%s",
			   Color::word, 
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "",
			   Color::end); 
	}

	virtual string format_out() const {
		string text_flags= flags_format(flags & ~F_TARGET_DYNAMIC);
		if (text_flags != "")
			text_flags += ' '; 
		string text_dep= dep->format_out(); 
		return fmt("%s[%s]",
			   text_flags,
			   text_dep); 
	}

	virtual string format_src() const {
		string text_flags= flags_format(flags & ~F_TARGET_DYNAMIC);
		if (text_flags != "")
			text_flags += ' '; 
		string text_dep= dep->format_src(); 
		string ret= fmt("%s[%s]",
				text_flags,
				text_dep); 
		if (top) { // RM
			ret += " : "; 
			ret += top->format_src();
		}
		return ret; 
	}

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

	Concat_Dep()
	/* An empty concatenation, i.e., a concatenation of zero dependencies */ 
	{  }

	Concat_Dep(Flags flags_, const Place places_[C_PLACED])
		/* The list of dependencies is empty */ 
		:  Dep(flags_, places_)
	{  }

	const vector <shared_ptr <const Dep> > &get_deps() const {
		return deps; 
	}

	vector <shared_ptr <const Dep> > &get_deps() {
		return deps; 
	}

	/* Append a dependency to the list */
	void push_back(shared_ptr <const Dep> dep)
	{
		deps.push_back(dep); 
	}

	virtual shared_ptr <const Dep> 
	instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const;

	virtual string format(Style, bool &quotes) const; 
	virtual string format_word() const; 
	virtual string format_out() const; 
	virtual string format_src() const; 

	virtual bool is_normalized() const; 
	/* A concatenated dependency is always normalized, regardless of
	 * whether the contained dependencies are normalized.  */ 

	virtual Target get_target() const;

	static shared_ptr <const Plain_Dep> concat(shared_ptr <const Plain_Dep> a,
						   shared_ptr <const Plain_Dep> b); 
	/* Concatenate two plain dependencies to a single plain
	 * dependency.  
	 * XXX how are errors handled?
	 */


	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     vector <shared_ptr <const Dep> > &deps); 
	/* Normalize this object's dependencies into a list of individual
	 * dependencies.  The generated dependencies are appended to
	 * DEPS which does not need to be empty on entry into
	 * this function.  */

	static void normalize_concat(shared_ptr <const Concat_Dep> dep,
				     vector <shared_ptr <const Dep> > &deps,
				     size_t start_index);
	/* Helper function.  Write result into DEPS,
	   concatenating all starting at the given index.  */


private:

	vector <shared_ptr <const Dep> > deps;
	/* The dependencies for each part.  May be empty in code, which is something
	 * that is not allowed in Stu code.  Otherwise, there is at
	 * least one element, which is either a Compound_Dep, or
	 * a normalized dependency.  */
};

class Compound_Dep
/* 
 * A list of dependencies that act as a unit, corresponding
 * syntactically to a list of dependencies in parentheses.  
 *
 * In terms of Stu source code, a compound dependency corresponds to
 *
 *         (X Y Z ...)
 */
	:  public Dep
{
public:

	Place place; 
	/* The place of the compound ; usually the opening parenthesis
	 * or brace.  May be empty to denote no place, in particular if
	 * this is a "logical" compound dependency not coming from a
	 * parenthesised expression.  */

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

	const vector <shared_ptr <const Dep> > &get_deps() const {
		return deps; 
	}

	vector <shared_ptr <const Dep> > &get_deps() {
		return deps; 
	}

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
	virtual string format_word() const; 
	virtual string format_out() const; 
	virtual string format_src() const; 

	virtual bool is_normalized() const {  return false;  }
	/* A compound dependency is never normalized */

	virtual Target get_target() const {  assert(false);  }

private:

	vector <shared_ptr <const Dep> > deps;
	/* The contained dependencies, in given order */ 
};

class Root_Dep
	:  public Dep
{
public:
	virtual shared_ptr <const Dep> instantiate(const map <string, string> &) const {
		return shared_ptr <const Dep> (make_shared <Root_Dep> ()); 
	}
	virtual bool is_unparametrized() const {  return false;  }
	virtual const Place &get_place() const {  return Place::place_empty;  }
	virtual string format(Style, bool &) const {  return "ROOT";  }
	virtual string format_word() const {  return "ROOT";  }
	virtual string format_out() const {  return "ROOT";  }
	virtual string format_src() const {  return "ROOT";  }
	virtual Target get_target() const {  return Target("");  }
	virtual bool is_normalized() const {  return true;  }
};

Dep::~Dep() { }

void Dep::normalize(shared_ptr <const Dep> dep,
		    vector <shared_ptr <const Dep> > &deps)
{
	if (to <Plain_Dep> (dep)) {
		deps.push_back(dep);
	} else if (to <Dynamic_Dep> (dep)) {
		shared_ptr <const Dynamic_Dep> dynamic_dep= to <Dynamic_Dep> (dep);
		vector <shared_ptr <const Dep> > deps_child;
		normalize(dynamic_dep->dep, deps_child);
		for (auto &d:  deps_child) {
			shared_ptr <Dep> dep_new= 
				make_shared <Dynamic_Dep> 
				(dynamic_dep->flags, dynamic_dep->places, d);
			deps.push_back(dep_new); 
		}
	} else if (to <Compound_Dep> (dep)) {
		shared_ptr <const Compound_Dep> compound_dep= to <Compound_Dep> (dep);
		for (auto &d:  compound_dep->get_deps()) {
			shared_ptr <Dep> dd= Dep::clone(d); 
			dd->add_flags(compound_dep, false);  
			normalize(dd, deps); 
		}
	} else if (to <Concat_Dep> (dep)) {
		shared_ptr <const Concat_Dep> concat_dep= to <Concat_Dep> (Dep::clone(dep)); 
		Concat_Dep::normalize_concat(concat_dep, deps);
	} else {
		assert(false);
	}
}

shared_ptr <const Dep> Dep::normalize_compound(shared_ptr <const Dep> dep)
{
	vector <shared_ptr <const Dep> > deps;

	normalize(dep, deps); 
	
	assert(deps.size() >= 1);
	
	if (deps.size() == 1) {
		return deps[0];
	} else {
		shared_ptr <Compound_Dep> ret= make_shared <Compound_Dep> (Place()); 
		swap(deps, ret->get_deps()); 
		return ret;
	}
}

shared_ptr <Dep> Dep::clone(shared_ptr <const Dep> dep)
{
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
		assert(false); 
		/* Bug:  Unhandled dependency type */ 
	}
}

void Dep::add_flags(shared_ptr <const Dep> dep, 
		    bool overwrite_places)
{
	for (int i= 0;  i < C_PLACED;  ++i) {
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

	for (int i= 0;  i < C_PLACED;  ++i) {
		assert(((flags & (1 << i)) == 0) == get_place_flag(i).empty()); 
	}

	const Plain_Dep *plain_this= dynamic_cast <const Plain_Dep *> (this);
	const Dynamic_Dep *dynamic_this= dynamic_cast <const Dynamic_Dep *> (this); 

	if (plain_this) {
		/* The F_TARGET_TRANSIENT flag is always set in the
		 * dependency flags, even though that is redundant.  */
		assert((plain_this->flags & F_TARGET_TRANSIENT) == (plain_this->place_param_target.flags)); 

		if (plain_this->variable_name != "") {
			assert((plain_this->place_param_target.flags & F_TARGET_TRANSIENT) == 0); 
			assert(plain_this->flags & F_VARIABLE); 
		}
	}

	if (dynamic_this) {
		assert(flags & F_TARGET_DYNAMIC); 
		dynamic_this->dep->check(); 
	} else {
		assert(!(flags & F_TARGET_DYNAMIC)); 
	}
}
#endif


Target Plain_Dep::get_target() const
{
	Target ret= place_param_target.unparametrized(); 
	ret.get_front_word_nondynamic() |= (word_t)(flags & F_TARGET_BYTE);
	return ret; 
}

string Plain_Dep::format_out() const 
{
	string f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT));
	if (f != "")
		f += ' '; 
	return fmt("%s%s%s%s",
		   f,
		   flags & F_VARIABLE ? "$[" : "",
		   place_param_target.format_out(),
		   flags & F_VARIABLE ? "]" : "");
}

string Plain_Dep::format_src() const 
{
	string f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT));
	if (f != "")
		f += ' '; 
	string ret= fmt("%s%s%s%s",
			f,
			flags & F_VARIABLE ? "$[" : "",
			place_param_target.format_src(),
			flags & F_VARIABLE ? "]" : "");
	if (top) { // RM
		ret += " : "; 
		ret += top->format_src();
	}
	return ret;
}

string Plain_Dep::format_word() const
{
	string f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT));
	if (f != "") 
		f += ' ';
	bool quotes= Color::quotes; 
	string t= place_param_target.format
		(f.empty() ? 0 : 
		 S_MARKERS, 
		 quotes);
	return fmt("%s%s%s%s%s%s%s%s",
		   Color::word, 
		   f,
		   flags & F_VARIABLE ? "$[" : "",
		   quotes ? "'" : "",
		   t,
		   quotes ? "'" : "",
		   flags & F_VARIABLE ? "]" : "",
		   Color::end); 
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

shared_ptr <const Dep> Plain_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dep> ret= make_shared <Plain_Dep> 
		(flags, places, *ret_target, place, variable_name);

	assert(ret_target->place_name.get_n() == 0); 
	string this_name= ret_target->place_name.unparametrized(); 

	if ((flags & F_VARIABLE) &&
	    this_name.find('=') != string::npos) {

		assert((ret_target->flags & F_TARGET_TRANSIENT) == 0); 
		place << fmt("dynamic variable %s must not be instantiated with parameter value that contains %s", 
			     dynamic_variable_format_word(this_name),
			     char_format_word('='));
		throw ERROR_LOGICAL; 
	}

	return ret;
}

shared_ptr <const Dep> 
Compound_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Compound_Dep> ret= make_shared <Compound_Dep> (flags, places, place);

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
			ret += ", ";
		ret += d->format(style, quotes); 
	}
	if (deps.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dep::format_word() const
{
	string ret;
	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_word(); 
	}
	if (deps.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dep::format_out() const
{
	string ret;
	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_out(); 
	}
	if (deps.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dep::format_src() const
{
	string ret;
	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_src(); 
	}
	if (deps.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

shared_ptr <const Dep> Concat_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Concat_Dep> ret= make_shared <Concat_Dep> (flags, places);

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
{
	string ret;

	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += '*';
		ret += d->format(style, quotes); 
	}

	return ret; 
}

string Concat_Dep::format_word() const
{
	string ret;

	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_word(); 
	}
	
	return ret; 
}

string Concat_Dep::format_out() const
{
	string ret;

	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_out(); 
	}
	
	return ret; 
}

string Concat_Dep::format_src() const
{
	string ret;

	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_src(); 
	}
	
	return ret; 
}

bool Concat_Dep::is_normalized() const
{
	for (auto &i:  deps) {
		if (i->is_normalized())
			continue;
		shared_ptr <const Compound_Dep> i_compound= 
			to <Compound_Dep> (i);			
		if (i_compound != nullptr) {
			for (auto &j:  i_compound->get_deps()) {
				if (! j->is_normalized()) 
					return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep,
				  vector <shared_ptr <const Dep> > &deps_) 
{
	normalize_concat(dep, deps_, 0); 
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep, 
				  vector <shared_ptr <const Dep> > &deps_,
				  size_t start_index) 
{
	assert(start_index < dep->deps.size()); 

	if (start_index + 1 == dep->deps.size()) {
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (to <Compound_Dep> (dd)) {
			shared_ptr <const Compound_Dep> compound_dep= to <Compound_Dep> (dd);
			for (const auto &d:  compound_dep->get_deps()) {
				assert(to <Plain_Dep> (d));
				normalize(d, deps_); 
			}
		} else if (to <Plain_Dep> (dd)) {
			deps_.push_back(dd); 
		} else if (to <Concat_Dep> (dd)) {
			normalize_concat(to <Concat_Dep> (dd), deps_);
		} else {
			assert(false); 
		}
	} else {
		vector <shared_ptr <const Dep> > vec1, vec2;
		normalize_concat(dep, vec2, start_index + 1); 
		shared_ptr <const Dep> dd= dep->deps.at(start_index); 
		if (to <Compound_Dep> (dd)) {
			shared_ptr <const Compound_Dep> compound_dep= to <Compound_Dep> (dd);
			for (const auto &d:  compound_dep->get_deps()) {
				normalize(d, vec1); 
			}
		} else if (to <Plain_Dep> (dd)) {
			shared_ptr <const Plain_Dep> dd_plain= to <Plain_Dep> (dd); 
			vec1.push_back(dd); 
		} else if (to <Dynamic_Dep> (dd)) {
			assert(false); // XXX implement case
		} else if (to <Concat_Dep> (dd)) {
			normalize_concat(to <Concat_Dep> (dd), vec1); 
		} else {
			assert(false); 
		}

		for (const auto &d1:  vec1) {
			for (const auto &d2:  vec2) {
				// XXX handle the case of D1 and D2
				// being other things than plain
				// dependencies (return a Concat_Dep of
				// them, eventually with flattening if
				// one if them is already a Concat_Dep)
				deps_.push_back(concat(to <Plain_Dep> (d1), 
						       to <Plain_Dep> (d2))); 
			}
		}
	}
}

Target Concat_Dep::get_target() const
{
	/* Dep::get_target() is not used for complex dependencies */
	assert(false);
}

shared_ptr <const Plain_Dep> Concat_Dep::concat(shared_ptr <const Plain_Dep> a,
						shared_ptr <const Plain_Dep> b)
{
	assert(a);
	assert(b); 

	assert(! a->place_param_target.place_name.is_parametrized());  // XXX allow
	assert(! b->place_param_target.place_name.is_parametrized());  // XXX allow 
	assert(a->variable_name == "");  // XXX test
	assert(b->variable_name == "");  // XXX test
	assert((b->flags & F_TARGET_TRANSIENT) == 0); // XXX test
	// XXX test all other flags 

	/*
	 * XXX Test
	 */

	/*
	 * Combine 
	 */ 

	Flags flags_combined= a->flags | b->flags; 

	Place_Name place_name_combined(a->place_param_target.place_name.unparametrized() +
				       b->place_param_target.place_name.unparametrized(),
				       a->place_param_target.place_name.place); 

	shared_ptr <Plain_Dep> ret= 
		make_shared <Plain_Dep> (flags_combined,
						Place_Param_Target(flags_combined & F_TARGET_TRANSIENT,
								   place_name_combined,
								   a->place_param_target.place),
						a->place,
					 ""); 
	// XXX set Dep::places
	
	return ret; 
}

#endif /* ! DEP_HH */
