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

	/* The first C_TRANSITIVE flags are transitive, i.e., inherited
	 * across transient targets  */
	C_TRANSITIVE       = 3,

	/* 
	 * Transitive flags
	 */ 

	/* (-p) When the dependency is newer than the target, don't rebuild */ 
	F_PERSISTENT       = 1 << I_PERSISTENT,  

	/* (-o) Don't create the dependency if it doesn't exist */
	F_OPTIONAL         = 1 << I_OPTIONAL,

	/* (-t) Trivial dependency */
	F_TRIVIAL          = 1 << I_TRIVIAL,

	/* 
	 * Intransitive flags
	 */ 

	/* Read content of file and add it as new dependencies.  Used
	 * only for [...[X]...]->X links. */
	F_READ             = 1 << I_READ,  

	/* ($[...]) Content of file is used as variable */ 
	F_VARIABLE         = 1 << I_VARIABLE,

	/* Used only in Link.flags in the second pass.  Not used for
	 * dependencies.  Means to override all trivial flags. */ 
	F_OVERRIDE_TRIVIAL = 1 << I_OVERRIDE_TRIVIAL,

	/* For dynamic dependencies, the file contains newline-separated
	 * filenames, without any markup  */ 
	F_NEWLINE_SEPARATED= 1 << I_NEWLINE_SEPARATED,

	/* For dynamic dependencies, the file contains NUL-separated
	 * filenames, without any markup  */ 
	F_ZERO_SEPARATED=    1 << I_ZERO_SEPARATED,
};

/* Characters representing the individual flags -- used in verbose mode
 * output */ 
const char *const FLAGS_CHARS= "pot`$*n0"; 

/* Get the flag index corresponding to a character */ 
int flag_get_index(char c)
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

/* 
 * Textual representation of a flags value.  To be shown before the
 * argument.  Empty when flags are empty. 
 */
string flags_format(Flags flags) 
{
	string ret= "";
	for (int i= 0;  i < C_ALL;  ++i)
		if (flags & (1 << i))
			ret += frmt("-%c ", FLAGS_CHARS[i]); 
	return ret;
}

/* 
 * A dependency, which can be simple or compound.  All dependencies
 * carry information about their place(s) of declaration.  
 *
 * Objects of this type are usually used with shared_ptr/unique_ptr. 
 */ 
class Dependency
{
public:

	virtual ~Dependency(); 
	virtual shared_ptr <Dependency> 
	instantiate(const map <string, string> &mapping) const= 0;
	virtual bool is_unparametrized() const= 0; 

	/* Returns all flags */
	virtual Flags get_flags()= 0;
	/* Checks all given bits */
	virtual bool has_flags(Flags flags_)= 0; 
	/* Sets the given bits for this dependency */
	virtual void add_flags(Flags flags_)= 0;

	/* Where the dependency as a whole is declared */ 
	virtual const Place &get_place() const= 0;

	/* Get the place of a single flag */ 
	virtual const Place &get_place_flag(int i) const= 0;

	virtual void set_place_flag(int i, const Place &place)= 0;

	virtual string format(Style, bool &quotes) const= 0; 
	virtual string format_word() const= 0; 
	virtual string format_out() const= 0; 

	/* Collapse the dependency into a single target, ignoring all
	 * flags */   
	virtual Param_Target get_single_target() const= 0;

#ifndef NDEBUG
	virtual void print() const= 0; 
#endif
};

class Base_Dependency
	:  public Dependency
{
public:

	Flags flags;

	Place places[C_TRANSITIVE]; 

	Base_Dependency(Flags flags_) 
		:  flags(flags_)
	{ }

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

/* 
 * A parametrized dependency denoting an individual target name.  Does
 * not cover dynamic dependencies.  
 */
class Direct_Dependency
	:  public Base_Dependency
{
public:

	/* Cannot be a root target */ 
	Place_Param_Target place_param_target; 
	
	/* The place where the dependency is declared */ 
	Place place;

	/* 
	 * With F_VARIABLE:  the name of the variable.
	 * Otherwise:  empty. 
	 */
	string name;
	
	/* Take the dependency place from the target place */ 
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_)
		:  Base_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place)
	{ 
		check(); 
	}

	/* Take the dependency place from the target place, with variable_name */ 
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const string &name_)
		:  Base_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place),
		   name(name_)
	{ 
		check(); 
	}

	/* Use an explicit dependency place */ 
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_)
		:  Base_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_)
	{ 
		assert((flags_ & F_READ) == 0); 
		check(); 
	}

	/* Use an explicit dependency place */ 
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_,
			  const Place &place_,
			  const string &name_)
		:  Base_Dependency(flags_),
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

#ifndef NDEBUG
	void print() const {
		string text= place_param_target.format_word();
		place <<
			frmt("%d %s", flags, text.c_str()); 
	}
#endif
};

/* A dynamic dependency */
class Dynamic_Dependency
	:  public Base_Dependency
{
public:

	/* Non-null */ 
	shared_ptr <Dependency> dependency;

	Dynamic_Dependency(Flags flags_,
			   shared_ptr <Dependency> dependency_)
		:  Base_Dependency(flags_), 
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

#ifndef NDEBUG
	void print() const {
		fprintf(stderr, "dynamic %d of:  ", flags);
		dependency->print(); 
	}
#endif
};

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
class Stack
{
public:
	/* Check the internal consistency of this object */ 
	void check() const {
		assert(k + 1 < CHAR_BIT * sizeof(int)); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			/* Only the (K+1) first bits may be set */ 
			assert((bits[i] & ~((1 << (k+1)) - 1)) == 0); 
		}
	}

	/* Depth is zero, the single flag is zero */ 
	Stack()
		:  k(0)
	{
		memset(bits, 0, sizeof(bits));
		check();
	}

	/* Depth is zero, the flag type is given */ 
	explicit Stack(Flags flags)
		:  k(0)
	{
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i]= ((flags & (1 << i)) != 0);
		}
		check(); 
	}

	/* Initalize to all-zero with the given depth K_ */ 
	Stack(unsigned k_, int zero) 
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

	/* Return the front dependency type, i.e. the lowest bits
	 * corresponding to the lowest level in the hierarchy. */
	Flags get_lowest() const {
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

	/* Get the flags when K == 0 */
	Flags get_one() const {
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

	/* Add the negation of the argument */ 
	void add_neg(Stack stack_) {
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

	/* K must be zero */ 
	void add_one_neg(Flags flags) {
		assert(k == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags >> i) & 1) ^ 1;
		}
	}

	/* K must be zero */ 
	void add_one_neg(Stack stack_) {
		assert(this->k == 0);
		assert(stack_.k == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= stack_.bits[i] ^ 1;
		}
	}

	/* Add a lowest level. (In-place change) */ 
	void push() {
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

	/* Remove the lowest level. (In-place change) */
	void pop() {
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

	/* The depth */
	unsigned k;

	/* The bits */ 
	unsigned bits[C_TRANSITIVE];
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
