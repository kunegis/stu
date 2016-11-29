#ifndef DEPENDENCY_HH
#define DEPENDENCY_HH

/*
 * Data types for representing dependencies. 
 */

#include <limits.h>

#include <map>
#include <memory>
#include <string>

#include "error.hh"
#include "target.hh"
#include "format.hh"

/*
 * Flags are represented in Stu files with a syntax that resembles
 * command line options, i.e., -p, -o, etc.  Here, flags are defined as
 * bit fields. 
 *
 * Each edge in the dependency graph is annotated with one
 * object of this type.  This contains bits related to what should be
 * done with the dependency, whether time is considered, etc.  The flags
 * are defined in such a way that the most simple dependency is
 * represented by zero, and each flag enables an optional feature.  
 *
 * The transitive bits effectively are set for tasks not to do.
 * Therefore, inverting them gives the bits for the tasks to do.   
 *
 * Declared as integer so arithmetic can be performed on it.
 */
typedef unsigned Flags; 
enum 
{
	/* The index of the flags, used for array indexing */ 
	I_PERSISTENT       = 0,
	I_OPTIONAL,         
	I_TRIVIAL,          
	I_READ,              
	I_VARIABLE,
	I_OVERRIDE_TRIVIAL,
	I_NEWLINE_SEPARATED,
	I_ZERO_SEPARATED,

	C_ALL,              

	C_TRANSITIVE       = 3,
	/* The first C_TRANSITIVE flags are transitive, i.e., inherited
	 * across transient targets  */

	/* What follows are the actual flag bits to be ORed together */ 

	/* 
	 * Transitive flags
	 */ 

	F_PERSISTENT       = 1 << I_PERSISTENT,  
	/* (-p) When the dependency is newer than the target, don't rebuild */ 

	F_OPTIONAL         = 1 << I_OPTIONAL,
	/* (-o) Don't create the dependency if it doesn't exist */

	F_TRIVIAL          = 1 << I_TRIVIAL,
	/* (-t) Trivial dependency */

	/* 
	 * Intransitive flags
	 */ 

	F_READ             = 1 << I_READ,  
	/* Read content of file and add it as new dependencies.  Used
	 * only for [...[X]...]->X links. */

	F_VARIABLE         = 1 << I_VARIABLE,
	/* ($[...]) Content of file is used as variable */ 

	F_OVERRIDE_TRIVIAL = 1 << I_OVERRIDE_TRIVIAL,
	/* Used only in Link.flags in the second pass.  Not used for
	 * dependencies.  Means to override all trivial flags. */ 

	F_NEWLINE_SEPARATED= 1 << I_NEWLINE_SEPARATED,
	/* For dynamic dependencies, the file contains newline-separated
	 * filenames, without any markup  */ 

	F_ZERO_SEPARATED=    1 << I_ZERO_SEPARATED,
	/* For dynamic dependencies, the file contains NUL-separated
	 * filenames, without any markup  */ 
};

const char *const FLAGS_CHARS= "pot`$*n0"; 
/* Characters representing the individual flags -- used in verbose mode
 * output */ 

int flag_get_index(char c)
/* 
 * Get the flag index corresponding to a character.
 */ 
{
	switch (c) {
	case 'p':  return I_PERSISTENT;
	case 'o':  return I_OPTIONAL;
	case 't':  return I_TRIVIAL;
	case 'n':  return I_NEWLINE_SEPARATED;
	case '0':  return I_ZERO_SEPARATED;
		
	default:
		assert(false);
		return 0;
	}
}

string flags_format(Flags flags) 
/* 
 * Textual representation of a flags value.  To be shown before the
 * argument.  Empty when flags are empty. 
 */
{
	string ret= "";
	for (int i= 0;  i < C_ALL;  ++i)
		if (flags & (1 << i))
			ret += frmt("-%c ", FLAGS_CHARS[i]); 
	return ret;
}

class Dependency
/* 
 * A dependency, which can be simple or compound.  All dependencies
 * carry information about their place(s) of declaration.  
 *
 * Objects of this type are usually used with shared_ptr/unique_ptr. 
 *
 * The flags only represent immediate flags.  Compound dependencies for
 * instance may contain additional inner flags. 
 */ 
{
public:

	virtual ~Dependency(); 
	virtual shared_ptr <Dependency> 
	instantiate(const map <string, string> &mapping) const= 0;
	virtual bool is_unparametrized() const= 0; 

	virtual Flags get_flags()= 0;
	/* Returns all flags */

	virtual bool has_flags(Flags flags_)= 0; 
	/* Return whether the dependency has all the given flags */

	virtual void add_flags(Flags flags_)= 0;
	/* Sets the given bits for this dependency */

	virtual const Place &get_place() const= 0;
	/* Where the dependency as a whole is declared */ 

	virtual const Place &get_place_flag(int i) const= 0;
	/* Get the place of a single flag */ 

	virtual void set_place_flag(int i, const Place &place)= 0;

	virtual string format(Style, bool &quotes) const= 0; 
	virtual string format_word() const= 0; 
	virtual string format_out() const= 0; 

	virtual Param_Target get_single_target() const= 0;
	/* Collapse the dependency into a single target, ignoring all
	 * flags.  Only if this is a simple dependency.  */   

	virtual bool is_simple() const= 0;
	/* A simple dependency is neither compound, nor concatenated */ 

#ifndef NDEBUG
	virtual void print() const= 0; 
	/* Write the dependency to standard output (only for debugging
	 * purposes)  */
#endif
};

class Single_Dependency
/*
 * A dependency with well-defined top-level flags.  This class is
 * intended to be inherited from and not instantiated directly. 
 */
	:  public Dependency
{
public:

	Flags flags;

	Place places[C_TRANSITIVE]; 
	/* For each transitive flag that is set, the place.  An empty
	 * place if a flag is not set  */

	Single_Dependency()
		:  flags(0)
	{  }

	Single_Dependency(Flags flags_) 
		:  flags(flags_)
	{ }

	Single_Dependency(Flags flags_, const Place places_[C_TRANSITIVE])
		:  flags(flags_)
	{
		assert(places != places_);
		assert(sizeof(places) == sizeof(places_)); 
		memcpy(places, places_, sizeof(places)); 
	}

	Flags get_flags() {
		return flags; 
 	}

	bool has_flags(Flags flags_)
	{
		return (flags & flags_) == flags_; 
	}

	void add_flags(Flags flags_)
	{
		flags |= flags_; 
	}

	const Place &get_place_flag(int i) const {
		assert(i >= 0 && i < C_TRANSITIVE);
		return places[i];
	}

	void set_place_flag(int i, const Place &place) {
		assert(i >= 0 && i < C_TRANSITIVE);
		places[i]= place; 
	}
};

class Direct_Dependency
/* 
 * A parametrized dependency denoting an individual target name.  Does
 * not cover dynamic dependencies.  
 */
	:  public Single_Dependency
{
public:

	Place_Param_Target place_param_target; 
	/* Cannot be a root target */ 
	
	Place place;
	/* The place where the dependency is declared */ 

	string name;
	/* With F_VARIABLE:  the name of the variable.
	 * Otherwise:  empty.  */
	
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_)
		/* Take the dependency place from the target place */ 
		:  Single_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const string &name_)
		/* Take the dependency place from the target place, with variable_name */ 
		:  Single_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place),
		   name(name_)
	{ 
		check(); 
	}

	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_)
		/* Use an explicit dependency place */ 
		:  Single_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_)
	{ 
		assert((flags_ & F_READ) == 0); 
		check(); 
	}

	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_,
			  const string &name_)
		/* Use an explicit dependency place */ 
		:  Single_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_),
		   name(name_)
	{ 
		assert((flags_ & F_READ) == 0); 
		check(); 
	}

	const Place &get_place() const {
		return place; 
	}

	shared_ptr <Dependency> instantiate(const map <string, string> &mapping) const;

	bool is_unparametrized() const {
		return place_param_target.place_name.get_n() == 0; 
	}

	void check() const {
		/* Must not be dynamic, since dynamic dependencies are
		 * represented using Dynamic_Dependency */ 
		assert(! place_param_target.type.is_dynamic());
		if (name != "") {
			assert(place_param_target.type == Type::FILE);
			assert(flags & F_VARIABLE); 
		}
	}

	string format(Style style, bool &quotes) const {
		string f= flags_format(flags & ~F_VARIABLE); 
		if (f != "")
			style |= S_MARKERS;
		string t= place_param_target.format(style, quotes);
		return fmt("%s%s%s%s%s%s",
			   f,
			   flags & F_VARIABLE ? "$[" : "",
			   quotes ? "'" : "",
			   t,
			   quotes ? "'" : "",
			   flags & F_VARIABLE ? "]" : "");
	}

	string format_word() const {
		string f= flags_format(flags & ~F_VARIABLE);
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

	string format_out() const {
		return fmt("%s%s%s%s",
			   flags_format(flags & ~F_VARIABLE),
			   flags & F_VARIABLE ? "$[" : "",
			   place_param_target.format_out(),
			   flags & F_VARIABLE ? "]" : "");
	}

	Param_Target get_single_target() const {
		return place_param_target.get_param_target();
	}

	virtual bool is_simple() const { return true; }

#ifndef NDEBUG
	void print() const {
		string text= place_param_target.format_word();
		place <<
			frmt("%d %s", flags, text.c_str()); 
	}
#endif
};

class Dynamic_Dependency
/* A dynamic dependency */
	:  public Single_Dependency
{
public:

	shared_ptr <Dependency> dependency;
	/* Non-null */ 

	Dynamic_Dependency(Flags flags_,
			   shared_ptr <Dependency> dependency_)
		:  Single_Dependency(flags_), 
		   dependency(dependency_)
	{
		assert((flags & F_READ) == 0); 
		assert((flags & F_VARIABLE) == 0); 
		assert(dependency_ != nullptr); 
	}

	shared_ptr <Dependency> 
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

	string format(Style, bool &quotes) const {
		quotes= false;
		bool quotes2= false;
		string s= dependency->format(S_MARKERS, quotes2);
		return fmt("%s[%s%s%s]",
			   quotes2 ? "'" : "",
			   s,
			   quotes2 ? "'" : "");
	}

	string format_word() const {
		bool quotes= false;
		string s= dependency->format(S_MARKERS, quotes);
		return fmt("%s%s[%s%s%s]%s",
			   Color::word, 
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "",
			   Color::end); 
	}

	string format_out() const {
		string text_flags= flags_format(flags);
		string text_dependency= dependency->format_out(); 
		return fmt("%s[%s]",
			   text_flags,
			   text_dependency); 
	}

	Param_Target get_single_target() const {
		Param_Target ret= dependency->get_single_target();
		++ ret.type;
		return ret; 
	}

	virtual bool is_simple() const { return true; }

#ifndef NDEBUG
	void print() const {
		fprintf(stderr, "dynamic %d of:  ", flags);
		dependency->print(); 
	}
#endif
};

class Concatenated_Dependency
/*
 * A dependency that is the concatenation of multiple dependencies. 
 */
	:  public Single_Dependency
{
public:

	/* The dependency as a whole does not have a place */ 

	Concatenated_Dependency()
	{
	}

	Concatenated_Dependency(Flags flags_, const Place places_[C_TRANSITIVE])
		:  Single_Dependency(flags_, places_)
	{
		/* The list of dependencies is empty */ 
	}

	/* Append a dependency to the list */
	void push_back(shared_ptr <Dependency> dependency)
	{
		dependencies.push_back(dependency); 
	}

	virtual bool is_simple() const { return false; }

	virtual shared_ptr <Dependency> 
	instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const;

	virtual string format(Style, bool &quotes) const; 
	virtual string format_word() const; 
	virtual string format_out() const; 

	virtual Param_Target get_single_target() const { assert(false); }
	/* Collapse the dependency into a single target, ignoring all
	 * flags.  Only if this is a simple dependency.  */   

#ifndef NDEBUG
	virtual void print() const; 
#endif

private:

	vector <shared_ptr <Dependency> > dependencies;
	/* The dependencies.  May be empty.  */
};

class Compound_Dependency
/* A list of dependencies that act as a unit, corresponding
 * syntactically to a list of dependencies in parentheses.  */
	:  public Single_Dependency
{
public:

	Place place; 
	/* The place of the compound ; usually the opening parenthesis
	 * or brace  */

	Compound_Dependency() 
	{  }
	
	Compound_Dependency(Flags flags_, const Place places_[C_TRANSITIVE])
		:  Single_Dependency(flags_, places_),
		   place()
	{
		/* The list of dependencies is empty */ 
	}

	Compound_Dependency(vector <shared_ptr <Dependency> > &&dependencies_)
		:  dependencies(dependencies_)
	{  }

	void push_back(shared_ptr <Dependency> dependency)
	{
		dependencies.push_back(dependency); 
	}

	virtual shared_ptr <Dependency> 
	instantiate(const map <string, string> &mapping) const;

	virtual bool is_unparametrized() const; 

	virtual const Place &get_place() const
	{
		return place; 
	}

	virtual string format(Style, bool &quotes) const; 
	virtual string format_word() const; 
	virtual string format_out() const; 

	virtual Param_Target get_single_target() const { assert(false); }
	/* Collapse the dependency into a single target, ignoring all
	 * flags.  Only if this is a simple dependency.  */   

	virtual bool is_simple() const { return false; }

#ifndef NDEBUG
	virtual void print() const; 
#endif

private:

	vector <shared_ptr <Dependency> > dependencies;
	/* The contained dependencies, in given order */ 
};

class Stack
/*
 * A stack of dependency bits.  Contains only transitive bits.
 * Lower bits denote relationships lower in the hierarchy.  The depth K
 * is the number of times the link is dynamic.  (K+1) bits are actually
 * stored for each flag.  The maximum depth K is therefore CHAR_BITS *
 * sizeof(int) - 1, i.e., at least 15 on standard C platforms, and 31 on
 * almost all used platforms.  
 *
 * As a general rule, indexes named I go over the F_COUNT different
 * flags (0..F_COUNT-1), and indexes named J go over the (K+1) levels of
 * depth (0..K).  
 *
 * Example:  a dynamic dependency  -o [ -p X]  would be represented by the stack of bits
 *   J=1:    bit 'o'
 *   J=0:    bit 'p'
 */
{
public:
	void check() const 
	/* Check the internal consistency of this object */ 
	{
		assert(k + 1 < CHAR_BIT * sizeof(int)); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			/* Only the (K+1) first bits may be set */ 
			assert((bits[i] & ~((1 << (k+1)) - 1)) == 0); 
		}
	}

	Stack()
		/* Depth is zero, the single flag is zero */ 
		:  k(0)
	{
		memset(bits, 0, sizeof(bits));
		check();
	}

	explicit Stack(Flags flags)
		/* Depth is zero, the flag type is given */ 
		:  k(0)
	{
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i]= ((flags & (1 << i)) != 0);
		}
		check(); 
	}

	Stack(unsigned k_, int zero) 
		/* Initalize to all-zero with the given depth K_ */ 
		:  k(k_)
	{
		(void) zero; 
		if (k >= CHAR_BIT * sizeof(int) - 1) {
			print_error("dynamic dependency recursion limit exceeded");
			throw ERROR_FATAL; 
		}
		memset(bits, 0, sizeof(bits));
		check();
	}

	Stack(shared_ptr <Dependency> dependency);

	unsigned get_k() const {
		return k;
	}

	Flags get_lowest() const 
	/* Return the front dependency type, i.e. the lowest bits
	 * corresponding to the lowest level in the hierarchy. */
	{
		check(); 
		Flags ret= 0;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			ret |= ((bits[i] & 1) << i);
		}
		return ret;
	}

	Flags get_highest() const {
		check(); 
		Flags ret= 0;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			ret |= (((bits[i] >> k) & 1) << i);
		}
		return ret;
	}

	Flags get_one() const 
	/* Get the flags when K == 0 */
	{
		assert(k == 0);
		check();
		Flags ret= 0;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			ret |= bits[i] << i;
		}
		return ret;
	}

	Flags get(int j) const {
		Flags ret= 0;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			ret |= ((bits[i] >> j) & 1) << i;
		}
		return ret;
	}

	void add(Stack stack_) {
		check(); 
		assert(stack_.get_k() == this->get_k()); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= stack_.bits[i]; 
		}
	}

	void add_neg(Stack stack_) 
	/* Add the negation of the argument */ 
	{
		check(); 
		assert(stack_.get_k() == this->get_k()); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= ((1 << (k+1)) - 1) ^ stack_.bits[i]; 
		}
		check(); 
	}

	void add_lowest(Flags flags) {
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags & (1 << i)) != 0);
		}
	}

	void add_highest(Flags flags) {
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags & (1 << i)) != 0) << k;
		}
	}

	void rem_highest(Flags flags) {
		check(); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] &= ~(((flags & (1 << i)) != 0) << k);
		}
	}

	void add_highest_neg(Flags flags) {
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags & (1 << i)) == 0) << k;
		}
	}

	void add_one_neg(Flags flags) 
	/* K must be zero */ 
	{
		assert(k == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags >> i) & 1) ^ 1;
		}
	}

	void add_one_neg(Stack stack_) 
	/* K must be zero */ 
	{
		assert(this->k == 0);
		assert(stack_.k == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= stack_.bits[i] ^ 1;
		}
	}

	void push() 
	/* Add a lowest level. (In-place change) */ 
	{
		assert(k < CHAR_BIT * sizeof(int)); 
		if (k == CHAR_BIT * sizeof(int) - 2) {
			print_error("dynamic dependency recursion limit exceeded");
			throw ERROR_FATAL; 
		}
		++k;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] <<= 1;
		}
	}

	void pop() 
	/* Remove the lowest level. (In-place change) */
	{
		assert(k > 0); 
		--k;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] >>= 1;
		}
	}

	string format() const {
		string ret= "";
		for (int j= k;  j >= 0;  --j) {
			Flags flags_j= get(j);
			ret += flags_format(flags_j);
			if (j)  ret += ',';
		}
		return fmt("{%s}", ret); 
	}

private:

	unsigned k;
	/* The depth */

	unsigned bits[C_TRANSITIVE];
	/* The bits */ 
};

Dependency::~Dependency() { }

shared_ptr <Dependency> Direct_Dependency
::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dependency> ret= make_shared <Direct_Dependency> 
		(flags, *ret_target, place, name);

	assert(ret_target->place_name.get_n() == 0); 
	string this_name= ret_target->place_name.unparametrized(); 

	if ((flags & F_VARIABLE) &&
	    this_name.find('=') != string::npos) {

		assert(ret_target->type == Type::FILE); 

		place << 
			fmt("dynamic variable %s must not be instantiated with parameter value that contains %s", 
			    dynamic_variable_format_word(this_name),
			    char_format_word('='));
		throw ERROR_LOGICAL; 
	}

	return ret;
}

shared_ptr <Dependency> 
Compound_Dependency::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Compound_Dependency> ret= make_shared <Compound_Dependency> (flags, places);

	for (const shared_ptr <Dependency> &d:  dependencies) {
		ret->push_back(d->instantiate(mapping));
	}
	
	return ret; 
}

bool Compound_Dependency::is_unparametrized() const
/* A compound dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <Dependency> d:  dependencies) {
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

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format(style, quotes); 
	}

	return fmt("(%s)", ret); 
}

string Compound_Dependency::format_word() const
{
	string ret;

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_word(); 
	}
	
	return fmt("(%s)", ret); 
}

string Compound_Dependency::format_out() const
{
	string ret;

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += ", ";
		ret += d->format_out(); 
	}
	
	return fmt("(%s)", ret); 
}

#ifndef NDEBUG
void Compound_Dependency::print() const
{
	place << format_word(); 
}
#endif

shared_ptr <Dependency> 
Concatenated_Dependency::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Concatenated_Dependency> ret= 
		make_shared <Concatenated_Dependency>
		(flags, places);

	for (const shared_ptr <Dependency> &d:  dependencies) {
		ret->push_back(d->instantiate(mapping)); 
	}

	return ret; 
}

bool Concatenated_Dependency::is_unparametrized() const
/* A concatenated dependency is parametrized when any of its contained 
 * dependency is parametrized.  */
{
	for (shared_ptr <Dependency> d:  dependencies) {
		if (! d->is_unparametrized())
			return false;
	}

	return true;
}

const Place &Concatenated_Dependency::get_place() const
/* Return the place of the first dependency, or an empty place */
{
	static Place place_empty;

	if (dependencies.empty())
		return place_empty;

	return dependencies.front()->get_place(); 
}

string Concatenated_Dependency::format(Style style, bool &quotes) const
{
	string ret;

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format(style, quotes); 
	}

	return ret; 
}

string Concatenated_Dependency::format_word() const
{
	string ret;

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_word(); 
	}
	
	return ret; 
}

string Concatenated_Dependency::format_out() const
{
	string ret;

	for (const shared_ptr <Dependency> &d:  dependencies) {
		if (! ret.empty())
			ret += '*';
		ret += d->format_out(); 
	}
	
	return ret; 
}

#ifndef NDEBUG
void Concatenated_Dependency::print() const
{
	if (dependencies.empty())
		fprintf(stderr, "empty\n"); 
	else
		dependencies.front()->get_place() << format_word(); 
}
#endif

#ifndef NDEBUG
void print_dependencies(const vector <shared_ptr <Dependency> > &dependencies)
{
	for (auto &i:  dependencies) {
		i->print(); 
	}
}
#endif /* ! NDEBUG */

Stack::Stack(shared_ptr <Dependency> dependency) 
{
	k= 0;
	memset(bits, 0, sizeof(bits));

	while (dynamic_pointer_cast <Dynamic_Dependency> (dependency)) {
		shared_ptr <Dynamic_Dependency> dynamic_dependency
			= dynamic_pointer_cast <Dynamic_Dependency> (dependency);
		add_lowest(dynamic_dependency->flags);

		push(); 
		dependency= dynamic_dependency->dependency; 
	}

	assert(dynamic_pointer_cast <Direct_Dependency> (dependency));
	shared_ptr <Direct_Dependency> direct_dependency=
		dynamic_pointer_cast <Direct_Dependency> (dependency);
	add_lowest(direct_dependency->flags); 
}

#endif /* ! DEPENDENCY_HH */
