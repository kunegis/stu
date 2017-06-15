#ifndef DEPENDENCY_HH
#define DEPENDENCY_HH

/*
 * Data types for representing dependencies.  Dependencies are the
 * central data structures in Stu, as all dependencies in the syntax of
 * a Stu file get mapped to Dependency objects.  
 *
 * Dependencies are polymorphous objects, and all dependencies derive
 * from the class Dependency, and are used via shared_ptr<>, except in
 * cases where access is read-only.  This is necessary in cases where a
 * member function has to access its own THIS pointer, because we can't
 * put THIS into a shared pointer. 
 *
 * All dependency classes allow parametrized targets.  
 */

#include <map>

#include "error.hh"
#include "target.hh"
#include "flags.hh"

class Dependency
/* 
 * The abstract base class for all dependencies.  Objects of this type
 * are used via shared_ptr<>.
 *
 * A dependency can be normalized or not.  A dependency is normalized if it
 * is one of:  
 *    - a single dependency;
 *    - a dynamic dependency containing a normalized dependency; 
 *    - a concatenated dependency, each of whose component is either a
 *      normalized dependency or a compound dependency of normalized
 *      dependencies.
 * Normalized dependencies are those used in practice.  A non-normalized dependency
 * can always be reduced to a normalized one. 
 * 
 * All dependencies carry information about their place(s) of declaration.  
 *
 * The flags only represent immediate flags.  Compound dependencies for
 * instance may contain additional inner flags. 
 *
 * Objects of type Dependency and subclasses are always handled through
 * shared_ptr<>.  All objects may have many persistent pointers to it,
 * so they are considered final, i.e., immutable, except if we just
 * created the object in which case we know that it is not shared.  
 */ 
{
public:

	Flags flags;
	// TODO in principle, we could save the type within FLAGS and
	// save on using the C++ polymorphic overhead. 

	Place places[C_PLACED]; 
	/* For each transitive flag that is set, the place.  An empty
	 * place if a flag is not set  */

	Dependency()
		:  flags(0) 
	{  }

	Dependency(Flags flags_) 
		:  flags(flags_)
	{ }

	Dependency(Flags flags_, const Place places_[C_PLACED])
		:  flags(flags_)
	{
		assert(places != places_);
		memcpy(places, places_, sizeof(places)); 
	}

	virtual ~Dependency(); 

	const Place &get_place_flag(int i) const {
		assert(i >= 0 && i < C_PLACED);
		return places[i];
	}

	void set_place_flag(int i, const Place &place) {
		assert(i >= 0 && i < C_PLACED);
		places[i]= place; 
	}

	void add_flags(shared_ptr <const Dependency> dependency, 
		       bool overwrite_places);
	/* Add the flags from DEPENDENCY.  Also copy over the
	 * corresponding places.  If a place is already given in THIS,
	 * only copy a place over if OVERWRITE_PLACES is set.  */

	virtual shared_ptr <const Dependency> instantiate(const map <string, string> &mapping) const= 0;
	virtual bool is_unparametrized() const= 0; 

	virtual const Place &get_place() const= 0;
	/* Where the dependency as a whole is declared */ 

	virtual string format(Style, bool &quotes) const= 0; 
	virtual string format_word() const= 0; 
	virtual string format_out() const= 0; 

	virtual Target2 get_target2() const= 0;
	/* Get the corresponding Target2 object.  Only called for
	 * non-compound and non-parametrized dependencies.  */

	virtual bool is_normalized() const= 0;
	/* Whether the dependency is normalized, according to the
	 * definition given above.  */

	static void make_normalized(vector <shared_ptr <const Dependency> > &dependencies, 
				    shared_ptr <const Dependency> dependency);
	/* Split the given DEPENDENCY into multiple DEPENDENCIES that do
	 * not contain compound dependencies (recursively). The result is written into DEPENDENCIES,
	 * which does not have to be empty.  */

	static shared_ptr <const Dependency> make_normalized_compound(shared_ptr <const Dependency> dependency);
	/* Return either a normalized version of this dependency, or a
	 * Compound_Dependency containing normalized dependencies */ 

	static shared_ptr <Dependency> clone(shared_ptr <const Dependency> dependency);
	/* A shallow clone.  */

	static shared_ptr <const Dependency> strip_dynamic(shared_ptr <const Dependency> d);
	/* Strip dynamic dependencies from the given dependency.
	 * Perform recursively:  If D is a dynamic dependency, return
	 * its contained dependency, otherwise return D.  Thus, never
	 * return null.  */
};

class Single_Dependency
/* 
 * A dependency denoting an individual target name.  Does not cover
 * dynamic dependencies.
 */
	:  public Dependency
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
	
	Single_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Single_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const string &variable_name_)
		/* Take the dependency place from the target place, with variable_name */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Single_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_)
		/* Use an explicit dependency place */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_)
	{ 
		check(); 
	}

	Single_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_,
			  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Single_Dependency(const Single_Dependency &single_dependency)
		:  Dependency(single_dependency.flags),
		   place_param_target(single_dependency.place_param_target),
		   place(single_dependency.place),
		   variable_name(single_dependency.variable_name)
	{  }

	const Place &get_place() const {
		return place; 
	}

	shared_ptr <const Dependency> instantiate(const map <string, string> &mapping) const;

	bool is_unparametrized() const {
		return place_param_target.place_name.get_n() == 0; 
	}

	void check() const {
		if (variable_name != "") {
			assert((place_param_target.flags & F_TARGET_TRANSIENT) == 0); 
			assert(flags & F_VARIABLE); 
		}
	}

	virtual string format(Style style, bool &quotes) const {
		string f= flags_format(flags & ~F_VARIABLE); 
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

	virtual string format_word() const {
		string f= flags_format(flags & ~F_VARIABLE);
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

	virtual string format_out() const {
		string f= flags_format(flags & ~F_VARIABLE);
		if (f != "")
			f += ' '; 
		return fmt("%s%s%s%s",
			   f,
			   flags & F_VARIABLE ? "$[" : "",
			   place_param_target.format_out(),
			   flags & F_VARIABLE ? "]" : "");
	}

	virtual bool is_normalized() const { return true;  }

	virtual Target2 get_target2() const;
};

class Dynamic_Dependency
/* A dynamic dependency */
	:  public Dependency
{
public:

	shared_ptr <const Dependency> dependency;
	/* The contained dependency.  Non-null. */ 

	Dynamic_Dependency(Flags flags_,
			   shared_ptr <const Dependency> dependency_)
		:  Dependency(flags_), 
		   dependency(dependency_)
	{
		assert((flags & F_VARIABLE) == 0); 
		assert(dependency_ != nullptr); 
	}

	Dynamic_Dependency(Flags flags_,
			   const Place places_[C_PLACED],
			   shared_ptr <const Dependency> dependency_)
		:  Dependency(flags_, places_),
		   dependency(dependency_)
	{
		assert((flags & F_VARIABLE) == 0); /* Variables cannot be dynamic */
		assert(dependency_ != nullptr); 
	}

	shared_ptr <const Dependency> 
	instantiate(const map <string, string> &mapping) const
	{
		return make_shared <Dynamic_Dependency> 
			(flags, dependency->instantiate(mapping));
	}

	bool is_unparametrized() const
	{
		return dependency->is_unparametrized(); 
	}

	const Place &get_place() const {
		return dependency->get_place(); 
	}

	virtual string format(Style, bool &quotes) const {
		quotes= false;
		bool quotes2= false;
		string s= dependency->format(S_MARKERS, quotes2);
		return fmt("%s[%s%s%s]",
			   quotes2 ? "'" : "",
			   s,
			   quotes2 ? "'" : "");
	}

	virtual string format_word() const {
		bool quotes= false;
		string s= dependency->format(S_MARKERS, quotes);
		return fmt("%s[%s%s%s]%s",
			   Color::word, 
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "",
			   Color::end); 
	}

	virtual string format_out() const {
		string text_flags= flags_format(flags);
		if (text_flags != "")
			text_flags += ' '; 
		string text_dependency= dependency->format_out(); 
		return fmt("%s[%s]",
			   text_flags,
			   text_dependency); 
	}

	virtual bool is_normalized() const {
		return dependency->is_normalized(); 
	}

	virtual Target2 get_target2() const;

	unsigned get_depth() const 
		/* The depth of the dependency, i.e., how many dynamic
		 * dependencies are stacked in a row.  */
	{
		if (dynamic_pointer_cast <const Dynamic_Dependency> (dependency)) {
			return 1 + dynamic_pointer_cast <const Dynamic_Dependency> (dependency)->get_depth(); 
		} else {
			return 1; 
		}
	}
};

class Concatenated_Dependency
/* 
 * A dependency that is the concatenation of multiple dependencies. 
 * The dependency as a whole does not have a place stored; the
 * place of the first sub-dependency is used.  
 *
 * In terms of Stu code, a concatenated dependency corresponds to
 *
 *         (( X )( Y )( Z )...)
 */ 
	:  public Dependency
{
public:

	Concatenated_Dependency()
	/* An empty concatenation, i.e., a concatenation of zero dependencies */ 
	{  }

	Concatenated_Dependency(Flags flags_, const Place places_[C_PLACED])
		/* The list of dependencies is empty */ 
		:  Dependency(flags_, places_)
	{  }

	const vector <shared_ptr <const Dependency> > &get_dependencies() const {
		return dependencies; 
	}

	vector <shared_ptr <const Dependency> > &get_dependencies() {
		return dependencies; 
	}

	/* Append a dependency to the list */
	void push_back(shared_ptr <const Dependency> dependency)
	{
		dependencies.push_back(dependency); 
	}

	void make_normalized();

	virtual shared_ptr <const Dependency> 
	instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const;

	virtual string format(Style, bool &quotes) const; 
	virtual string format_word() const; 
	virtual string format_out() const; 

	virtual bool is_normalized() const; 
	/* A concatenated dependency is always normalized, regardless of
	 * whether the contained dependencies are normalized.  */ 

	virtual Target2 get_target2() const;

private:

	vector <shared_ptr <const Dependency> > dependencies;
	/* The dependencies.  May be empty in code, which is something
	 * that is not allowed in Stu code.  */
};

class Compound_Dependency
/* 
 * A list of dependencies that act as a unit, corresponding
 * syntactically to a list of dependencies in parentheses.  
 *
 * In terms of Stu source code, a compound dependency corresponds to
 *
 *         (X Y Z ...)
 */
	:  public Dependency
{
public:

	Place place; 
	/* The place of the compound ; usually the opening parenthesis
	 * or brace.  May be empty to denote no place, in particular if
	 * this is a "logical" compound dependency not coming from a
	 * parenthesised expression.  */

	Compound_Dependency(const Place &place_) 
		/* Empty, with zero dependencies */
		:  place(place_)
	{  }
	
	Compound_Dependency(Flags flags_, const Place places_[C_PLACED], const Place &place_)
		:  Dependency(flags_, places_),
		   place(place_)
	{
		/* The list of dependencies is empty */ 
	}

	Compound_Dependency(vector <shared_ptr <const Dependency> > &&dependencies_, 
			    const Place &place_)
		:  place(place_),
		   dependencies(dependencies_)
	{  }

	const vector <shared_ptr <const Dependency> > &get_dependencies() const {
		return dependencies; 
	}

	vector <shared_ptr <const Dependency> > &get_dependencies() {
		return dependencies; 
	}

	void push_back(shared_ptr <const Dependency> dependency)
	{
		dependencies.push_back(dependency); 
	}

	virtual shared_ptr <const Dependency> instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const
	{
		return place; 
	}

	virtual string format(Style, bool &quotes) const; 
	virtual string format_word() const; 
	virtual string format_out() const; 

	virtual bool is_normalized() const {  return false;  }
	/* A compound dependency is never normalized */

	virtual Target2 get_target2() const {  assert(false);  }

private:

	vector <shared_ptr <const Dependency> > dependencies;
	/* The contained dependencies, in given order */ 
};

class Root_Dependency
	:  public Dependency
{
public:
	virtual shared_ptr <const Dependency> instantiate(const map <string, string> &) const {
		return shared_ptr <const Dependency> (this); 
	}
	virtual bool is_unparametrized() const {  return false;  }
	virtual const Place &get_place() const {  return Place::place_empty;  }
	virtual string format(Style, bool &) const {  return "ROOT";  }
	virtual string format_word() const {  return "ROOT";  }
	virtual string format_out() const {  return "ROOT";  }
	virtual Target2 get_target2() const {  return Target2("");  }
	virtual bool is_normalized() const {  return true;  }
};

Dependency::~Dependency() { }

void Dependency::make_normalized(vector <shared_ptr <const Dependency> > &dependencies, 
				 shared_ptr <const Dependency> dependency)
{
	if (dynamic_pointer_cast <const Single_Dependency> (dependency)) {
		dependencies.push_back(dependency);

	} else if (dynamic_pointer_cast <const Dynamic_Dependency> (dependency)) {
		shared_ptr <const Dynamic_Dependency> dynamic_dependency= 
			dynamic_pointer_cast <const Dynamic_Dependency> (dependency);
		vector <shared_ptr <const Dependency> > dependencies_child;
		make_normalized(dependencies_child, dynamic_dependency->dependency);
		for (auto &d:  dependencies_child) {
			shared_ptr <Dependency> dependency_new= 
				make_shared <Dynamic_Dependency> 
				(dynamic_dependency->flags, dynamic_dependency->places, d);
			dependencies.push_back(dependency_new); 
		}

	} else if (dynamic_pointer_cast <const Compound_Dependency> (dependency)) {
		shared_ptr <const Compound_Dependency> compound_dependency=
			dynamic_pointer_cast <const Compound_Dependency> (dependency);
		for (auto &d:  compound_dependency->get_dependencies()) {
			shared_ptr <Dependency> dd= Dependency::clone(d); 
			dd->add_flags(compound_dependency, false);  
			make_normalized(dependencies, dd); 
		}

	} else if (dynamic_pointer_cast <const Concatenated_Dependency> (dependency)) {

		shared_ptr <Concatenated_Dependency> d= 
			dynamic_pointer_cast <Concatenated_Dependency> (Dependency::clone(dependency)); 
		d->make_normalized(); 
		// TODO use D 

	} else {
		/* Bug:  Unhandled dependency type */ 
		assert(false);
	}
}

shared_ptr <const Dependency> Dependency::make_normalized_compound(shared_ptr <const Dependency> dependency)
{
	vector <shared_ptr <const Dependency> > dependencies;

	make_normalized(dependencies, dependency); 
	
	assert(dependencies.size() >= 1);
	
	if (dependencies.size() == 1) {
		return dependencies[0];
	} else {
		shared_ptr <Compound_Dependency> ret= make_shared <Compound_Dependency> (Place()); 
		swap(dependencies, ret->get_dependencies()); 
		return ret;
	}
}

shared_ptr <Dependency> Dependency::clone(shared_ptr <const Dependency> dependency)
{
	// TODO we can optimize this by returning the object itself if
	// it is unique.  This however violates const-correctness. 
//	if (dependency.unique())
//		return dependency; 

	if (dynamic_pointer_cast <const Single_Dependency> (dependency)) {
		return make_shared <Single_Dependency> (* dynamic_pointer_cast <const Single_Dependency> (dependency)); 
	} else if (dynamic_pointer_cast <const Dynamic_Dependency> (dependency)) {
		return make_shared <Dynamic_Dependency> (* dynamic_pointer_cast <const Dynamic_Dependency> (dependency)); 
	} else if (dynamic_pointer_cast <const Compound_Dependency> (dependency)) {
		return make_shared <Compound_Dependency> (* dynamic_pointer_cast <const Compound_Dependency> (dependency)); 
	} else if (dynamic_pointer_cast <const Concatenated_Dependency> (dependency)) {
		return make_shared <Concatenated_Dependency> (* dynamic_pointer_cast <const Concatenated_Dependency> (dependency)); 
	} else if (dynamic_pointer_cast <const Root_Dependency> (dependency)) {
		return make_shared <Root_Dependency> (* dynamic_pointer_cast <const Root_Dependency> (dependency)); 
	} else {
		assert(false); 
		/* Bug:  Unhandled dependency type */ 
	}
}

void Dependency::add_flags(shared_ptr <const Dependency> dependency, 
			   bool overwrite_places)
{
	for (int i= 0;  i < C_PLACED;  ++i) {
		if (dependency->flags & (1 << i)) {
			if (overwrite_places || ! (this->flags & (1 << i))) {
				this->set_place_flag(i, dependency->get_place_flag(i)); 
			}
		}
	}
	this->flags |= dependency->flags; 
}

shared_ptr <const Dependency> Dependency::strip_dynamic(shared_ptr <const Dependency> d)
{
	assert(d != nullptr); 
	while (dynamic_pointer_cast <const Dynamic_Dependency> (d)) {
		d= dynamic_pointer_cast <const Dynamic_Dependency> (d)->dependency;
	}
	assert(d != nullptr); 
	return d;
}

Target2 Single_Dependency::get_target2() const
{
	return place_param_target.unparametrized(); 
}

Target2 Dynamic_Dependency::get_target2() const
{
	string text;
	
	const Dependency *d= this; 
	
	while (dynamic_cast <const Dynamic_Dependency *> (d)) {
		Flags f= F_TARGET_DYNAMIC; 
		f |= d->flags; 
		text += (char) f; 
		d= dynamic_cast <const Dynamic_Dependency *> (d)->dependency.get(); 
	}
	assert(dynamic_cast <const Single_Dependency *> (d)); 
	const Single_Dependency *sin= dynamic_cast <const Single_Dependency *> (d); 
	text += sin->place_param_target.unparametrized().get_text();

	return Target2(text); 
}

shared_ptr <const Dependency> Single_Dependency
::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dependency> ret= make_shared <Single_Dependency> 
		(flags, *ret_target, place, variable_name);

	assert(ret_target->place_name.get_n() == 0); 
	string this_name= ret_target->place_name.unparametrized(); 

	if ((flags & F_VARIABLE) &&
	    this_name.find('=') != string::npos) {

		assert((ret_target->flags & F_TARGET_TRANSIENT) == 0); 

		place << 
			fmt("dynamic variable %s must not be instantiated with parameter value that contains %s", 
			    dynamic_variable_format_word(this_name),
			    char_format_word('='));
		throw ERROR_LOGICAL; 
	}

	return ret;
}

shared_ptr <const Dependency> 
Compound_Dependency::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Compound_Dependency> ret= 
		make_shared <Compound_Dependency> 
		(flags, places, place);

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		ret->push_back(d->instantiate(mapping));
	}
	
	return ret; 
}

bool Compound_Dependency::is_unparametrized() const
/* A compound dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dependency> d:  dependencies) {
		if (! d->is_unparametrized())
			return false;
	}

	return true;
}

string Compound_Dependency::format(Style style, bool &) const
/* Ignore QUOTES, as everything inside the parentheses will not need
 * it.  */  
{
	string ret;

	bool quotes= false;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format(style, quotes); 
	}

	return fmt("(%s)", ret); 
}

string Compound_Dependency::format_word() const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_word(); 
	}
	
	return fmt("(%s)", ret); 
}

string Compound_Dependency::format_out() const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_out(); 
	}
	
	return fmt("(%s)", ret); 
}

shared_ptr <const Dependency> 
Concatenated_Dependency::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Concatenated_Dependency> ret= 
		make_shared <Concatenated_Dependency>
		(flags, places);

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		ret->push_back(d->instantiate(mapping)); 
	}

	return ret; 
}

bool Concatenated_Dependency::is_unparametrized() const
/* A concatenated dependency is parametrized when any of its contained 
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dependency> d:  dependencies) {
		if (! d->is_unparametrized())
			return false;
	}

	return true;
}

const Place &Concatenated_Dependency::get_place() const
/* Return the place of the first dependency, or an empty place */
{
	if (dependencies.empty())
		return Place::place_empty;

	return dependencies.front()->get_place(); 
}

string Concatenated_Dependency::format(Style style, bool &quotes) const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format(style, quotes); 
	}

	return ret; 
}

string Concatenated_Dependency::format_word() const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_word(); 
	}
	
	return ret; 
}

string Concatenated_Dependency::format_out() const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_out(); 
	}
	
	return ret; 
}

bool Concatenated_Dependency::is_normalized() const
{
	for (auto &i:  dependencies) {
		if (i->is_normalized())
			continue;
		shared_ptr <const Compound_Dependency> i_compound= 
			dynamic_pointer_cast <const Compound_Dependency> (i);			
		if (i_compound != nullptr) {
			for (auto &j:  i_compound->get_dependencies()) {
				if (! j->is_normalized()) 
					return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

void Concatenated_Dependency::make_normalized() 
{
	assert(0);  // actually implement it
#if 0
	for (size_t i= 0;  i < dependencies.size();  ++i) {
		shared_ptr <const Dependency> d_normalized_compound= 
			make_normalized_compound(dependencies[i]);
		dependencies[i]= d_normalized_compound; 
	}
#endif /* 0 */ 
}

Target2 Concatenated_Dependency::get_target2() const
{
	assert(false);
}

#endif /* ! DEPENDENCY_HH */
