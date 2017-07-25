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

/*
 * A dependency can be normalized or not.  A dependency is normalized if it
 * is one of:  
 *    - a plain dependency (file or transient);
 *    - a dynamic dependency containing a normalized dependency; 
 *    - a concatenated dependency, each of whose component is either a
 *      normalized dependency or a compound dependency of normalized
 *      dependencies.
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
 * A dependency that is not simple is complex.  I.e.m a complex
 * dependency involves concatenation and/or compound dependencies. 
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
 * The flags only represent immediate flags.  Compound dependencies for
 * instance may contain additional inner flags. 
 *
 * Objects of type Dependency and subclasses are always handled through
 * shared_ptr<>.  All objects may have many persistent pointers to it,
 * so they are considered final, i.e., immutable, except if we just
 * created the object in which case we know that it is not shared.
 * Therefore, we always use shared_ptr <const ...>, except when we just
 * created the dependency.  All dependencies are created via
 * make_shared<>. 
 */ 
{
public:

	Flags flags;

	Place places[C_PLACED]; 
	/* For each transitive flag that is set, the place.  An empty
	 * place if a flag is not set  */

	Dependency()
		:  flags(0) 
	{  }

	Dependency(Flags flags_) 
	/* TODO This constructor probably needs to be removed, because when
	 * FLAGS is set, PLACES should always also be set.  */
		:  flags(flags_)
	{  }

	Dependency(Flags flags_, const Place places_[C_PLACED])
		:  flags(flags_)
	{
		assert(places != places_);
		for (int i= 0;  i < C_PLACED;  ++i)
			places[i]= places_[i]; 
	}

	virtual ~Dependency(); 

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

	void add_flags(shared_ptr <const Dependency> dependency, 
		       bool overwrite_places);
	/* Add the flags from DEPENDENCY.  Also copy over the
	 * corresponding places.  If a place is already given in THIS,
	 * only copy a place over if OVERWRITE_PLACES is set.  */

#ifndef NDEBUG
	void check() const;
#else
	void check() const {  }
#endif		

	virtual shared_ptr <const Dependency> instantiate(const map <string, string> &mapping) const= 0;
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

	static void make_normalized(vector <shared_ptr <const Dependency> > &dependencies, 
				    shared_ptr <const Dependency> dependency);
	/* Split the given DEPENDENCY into multiple DEPENDENCIES that do
	 * not contain compound dependencies (recursively). The
	 * resulting dependencies are appended DEPENDENCIES, which does
	 * not have to be empty on entering the function.  */

	static shared_ptr <const Dependency> make_normalized_compound(shared_ptr <const Dependency> dependency);
	/* Return either a normalized version of this dependency, or a
	 * Compound_Dependency containing normalized dependencies */ 

	static shared_ptr <Dependency> clone(shared_ptr <const Dependency> dependency);
	/* A shallow clone */

	static shared_ptr <const Dependency> strip_dynamic(shared_ptr <const Dependency> d);
	/* Strip dynamic dependencies from the given dependency.
	 * Perform recursively:  If D is a dynamic dependency, return
	 * its contained dependency, otherwise return D.  Thus, never
	 * return null.  */
};

class Plain_Dependency
/* 
 * A dependency denoting an individual target name, which can be a file
 * or a transient.  
 *
 * When the target is a transient, the dependency flags have the
 * F_TARGET_TRANSIENT bit set, which is redundant.  No other Dependency
 * type has the F_TARGET_TRANSIENT flag set.
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

	// TODO in the following, deprecate all constructors that have a
	// FLAGS parameter without a PLACES parameter. 

	explicit Plain_Dependency(const Place_Param_Target &place_param_target_)
		:  Dependency(place_param_target_.flags),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{
		check(); 
	}
	
	Plain_Dependency(Flags flags_,
			 const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Plain_Dependency(Flags flags_,
			 const Place places_[C_PLACED],
			 const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Dependency(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Plain_Dependency(Flags flags_,
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

	Plain_Dependency(Flags flags_,
			 const Place_Param_Target &place_param_target_,
			 const Place &place_)
		/* Use an explicit dependency place */ 
		:  Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_)
	{ 
		check(); 
	}

	Plain_Dependency(Flags flags_,
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

	Plain_Dependency(Flags flags_,
			  const Place places_[C_PLACED],
			  const Place_Param_Target &place_param_target_,
			  const Place &place_,
			  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dependency(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Plain_Dependency(Flags flags_,
			  const Place places_[C_PLACED],
			  const Place_Param_Target &place_param_target_,
			  const string &variable_name_)
		/* Use an explicit dependency place */ 
		:  Dependency(flags_, places_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place),
		   variable_name(variable_name_)
	{ 
		check(); 
	}

	Plain_Dependency(const Plain_Dependency &plain_dependency)
		:  Dependency(plain_dependency),
		   place_param_target(plain_dependency.place_param_target),
		   place(plain_dependency.place),
		   variable_name(plain_dependency.variable_name)
	{  }

	const Place &get_place() const {
		return place; 
	}

	shared_ptr <const Dependency> instantiate(const map <string, string> &mapping) const;

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

class Dynamic_Dependency
/*
 * The Dependency::flags field does *not* have the F_TARGET_DYNAMIC
 * set. 
 */
// TODO make the F_TARGET_DYNAMIC field always be set in the
// Dependency::flags field. 
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
		return fmt("[%s%s%s]",
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

	virtual string format_src() const {
		string text_flags= flags_format(flags);
		if (text_flags != "")
			text_flags += ' '; 
		string text_dependency= dependency->format_src(); 
		return fmt("%s[%s]",
			   text_flags,
			   text_dependency); 
	}

	virtual bool is_normalized() const {
		return dependency->is_normalized(); 
	}

	virtual Target get_target() const;

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
 *         ( X )( Y )( Z )...
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

	void make_normalized_concatenated(vector <shared_ptr <const Dependency> > &dependencies) const; 
	/* Normalized this object's dependencies into a list of simple
	 * dependencies.  The generated dependencies are appended to
	 * DEPENDENCIES which does not need to be empty on entry into
	 * this function.  */

	void make_normalized_concatenated(vector <shared_ptr <const Dependency> > &dependencies,
					  size_t start_index) const;
	/* Helper function.  Write result into DEPENDENCIES,
	   concatenating all starting at the given index.  */

	virtual shared_ptr <const Dependency> 
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

	static shared_ptr <const Plain_Dependency> concatenate(shared_ptr <const Plain_Dependency> a,
								shared_ptr <const Plain_Dependency> b); 

private:

	vector <shared_ptr <const Dependency> > dependencies;
	/* The dependencies for each part.  May be empty in code, which is something
	 * that is not allowed in Stu code.  Otherwise, there is at
	 * least one element, which is either a Compound_Dependency, or
	 * a normalized dependency.  */
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
	virtual string format_src() const; 

	virtual bool is_normalized() const {  return false;  }
	/* A compound dependency is never normalized */

	virtual Target get_target() const {  assert(false);  }

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
	virtual string format_src() const {  return "ROOT";  }
	virtual Target get_target() const {  return Target("");  }
	virtual bool is_normalized() const {  return true;  }
};

Dependency::~Dependency() { }

void Dependency::make_normalized(vector <shared_ptr <const Dependency> > &dependencies, 
				 shared_ptr <const Dependency> dependency)
{
	if (dynamic_pointer_cast <const Plain_Dependency> (dependency)) {
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

		shared_ptr <Concatenated_Dependency> concatenated_dependency= 
			dynamic_pointer_cast <Concatenated_Dependency> (Dependency::clone(dependency)); 
		concatenated_dependency->make_normalized_concatenated(dependencies);

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
	if (dynamic_pointer_cast <const Plain_Dependency> (dependency)) {
		return make_shared <Plain_Dependency> (* dynamic_pointer_cast <const Plain_Dependency> (dependency)); 
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

#ifndef NDEBUG
void Dependency::check() const
{
	for (int i= 0;  i < C_PLACED;  ++i) {
		assert(((flags & (1 << i)) == 0) == get_place_flag(i).empty()); 
	}

	const Plain_Dependency *plain_this= dynamic_cast <const Plain_Dependency *> (this);
	const Dynamic_Dependency *dynamic_this= dynamic_cast <const Dynamic_Dependency *> (this); 

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
		dynamic_this->dependency->check(); 
	}
}
#endif


Target Plain_Dependency::get_target() const
{
	Target ret= place_param_target.unparametrized(); 
	ret.get_front_byte_nondynamic() |= (char)(unsigned char)(flags & F_TARGET_BYTE);
	return ret; 
}

string Plain_Dependency::format_out() const 
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

string Plain_Dependency::format_src() const 
{
	string f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT));
	if (f != "")
		f += ' '; 
	return fmt("%s%s%s%s",
		   f,
		   flags & F_VARIABLE ? "$[" : "",
		   place_param_target.format_src(),
		   flags & F_VARIABLE ? "]" : "");
}

string Plain_Dependency::format_word() const
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

Target Dynamic_Dependency::get_target() const
{
	string text;
	const Dependency *d= this; 
	while (dynamic_cast <const Dynamic_Dependency *> (d)) {
		Flags f= F_TARGET_DYNAMIC; 
		assert((d->flags & F_TARGET_DYNAMIC) == 0); 
		f |= d->flags & F_TARGET_BYTE; 
		text += (char)(unsigned char)f; 
		d= dynamic_cast <const Dynamic_Dependency *> (d)->dependency.get(); 
	}
	assert(dynamic_cast <const Plain_Dependency *> (d)); 
	const Plain_Dependency *sin= dynamic_cast <const Plain_Dependency *> (d); 
	Flags f= sin->flags & F_TARGET_BYTE;
	text += (char)(unsigned char)f; 
	
	// TODO in next line, copy the string directly from one string
	// object to another, without passing through a temporary string
	// object. 
	text += sin->place_param_target.unparametrized().get_name_nondynamic(); 
	
	return Target(text); 
}

shared_ptr <const Dependency> Plain_Dependency
::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dependency> ret= make_shared <Plain_Dependency> 
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
	if (dependencies.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dependency::format_word() const
{
	string ret;
	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_word(); 
	}
	if (dependencies.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dependency::format_out() const
{
	string ret;
	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_out(); 
	}
	if (dependencies.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
}

string Compound_Dependency::format_src() const
{
	string ret;
	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_src(); 
	}
	if (dependencies.size() != 1) 
		ret= fmt("(%s)", ret); 
	return ret;
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

string Concatenated_Dependency::format_src() const
{
	string ret;

	for (const shared_ptr <const Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_src(); 
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

void Concatenated_Dependency::make_normalized_concatenated(vector <shared_ptr <const Dependency> > &dependencies_) const
{
	make_normalized_concatenated(dependencies_, 0); 
}

void Concatenated_Dependency::make_normalized_concatenated(vector <shared_ptr <const Dependency> > &dependencies_,
							   size_t start_index) const
{
	assert(start_index < dependencies.size()); 

	if (start_index + 1 == dependencies.size()) {
		if (dynamic_pointer_cast <const Compound_Dependency> (dependencies.at(start_index))) {
			shared_ptr <const Compound_Dependency> compound_dependency= 
				dynamic_pointer_cast <const Compound_Dependency> (dependencies.at(start_index));
			for (const auto &d:  compound_dependency->get_dependencies()) {
				assert(dynamic_pointer_cast <const Plain_Dependency> (d));
				dependencies_.push_back(d); 
			}
		} else {
			assert(dynamic_pointer_cast <const Plain_Dependency> (dependencies.at(start_index)));
			dependencies_.push_back(dependencies.at(start_index)); 
		}
	} else {
		vector <shared_ptr <const Dependency> > vec;
		make_normalized_concatenated(vec, start_index + 1); 
		if (dynamic_pointer_cast <const Compound_Dependency> (dependencies.at(start_index))) {
			shared_ptr <const Compound_Dependency> compound_dependency= 
				dynamic_pointer_cast <const Compound_Dependency> (dependencies.at(start_index));
			for (const auto &d:  compound_dependency->get_dependencies()) {
				shared_ptr <const Plain_Dependency> d_plain=
					dynamic_pointer_cast <const Plain_Dependency> (d); 
				for (const auto &e:  vec) {
					shared_ptr <const Plain_Dependency> e_plain=
						dynamic_pointer_cast <const Plain_Dependency> (e); 
					dependencies_.push_back(concatenate(d_plain, e_plain)); 
				}
			}
		} else {
			shared_ptr <const Plain_Dependency> d_plain=
				dynamic_pointer_cast <const Plain_Dependency> (dependencies.at(start_index)); 
			for (const auto &e:  vec) {
				shared_ptr <const Plain_Dependency> e_plain=
					dynamic_pointer_cast <const Plain_Dependency> (e); 
				dependencies_.push_back(concatenate(d_plain, e_plain)); 
			}
		}
	}
}

Target Concatenated_Dependency::get_target() const
{
	/* Dependency::get_target() is not used for complex dependencies */
	assert(false);
}

shared_ptr <const Plain_Dependency> Concatenated_Dependency::concatenate(shared_ptr <const Plain_Dependency> a,
									 shared_ptr <const Plain_Dependency> b)
{
	assert(a);
	assert(b); 

	assert(! a->place_param_target.place_name.is_parametrized()); 
	assert(! b->place_param_target.place_name.is_parametrized()); 
	assert(a->variable_name == "");  // TEST
	assert(b->variable_name == "");  // TEST
	assert((b->flags & F_TARGET_TRANSIENT) == 0); // TEST
	// TODO test all other flags 

	Flags flags_combined= a->flags | b->flags; 

	Place_Name place_name_combined(a->place_param_target.place_name.unparametrized() +
				       b->place_param_target.place_name.unparametrized(),
				       a->place_param_target.place_name.place); 

	shared_ptr <Plain_Dependency> ret= 
		make_shared <Plain_Dependency> (flags_combined,
						Place_Param_Target(flags_combined & F_TARGET_TRANSIENT,
								   place_name_combined,
								   a->place_param_target.place),
						a->place,
						""); 
	// XXX set Dependency::places
	
	return ret; 
}

#endif /* ! DEPENDENCY_HH */
