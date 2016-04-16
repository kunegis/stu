#ifndef DEPENDENCY_HH
#define DEPENDENCY_HH

/* Data types for representing dependencies. 
 */

#include <limits.h>

#include <map>
#include <memory>
#include <string>

#include "error.hh"
#include "target.hh"

/* The flags.  Each edge in the dependency graph is annotated with one
 * object of this type.  This contains bits related to what should be
 * done with the dependency, whether time is considered, etc.  The flags
 * are defined in such a way that the most simple dependency is
 * represented by zero, and all flag enable an optional feature.  
 *
 * The transitive bits effectively are set for tasks not to do.
 * Therefore, inverting them gives the bits for the tasks to do.   
 *
 * Declared as int so arithmetic can be performed on it.
 */
typedef unsigned Flags; 
enum 
{
	/* Transitive bits */ 

	/* (!) When the dependency is newer than the target, don't rebuild */ 
	F_EXISTENCE        = (1 << 0),  

	/* (?) Don't create the dependency if it doesn't exist */
	F_OPTIONAL         = (1 << 1),

	/* (&) Trivial dependency */
	F_TRIVIAL          = (1 << 2),

	/* Intransitive bits */ 

	/* Read content of file and add it as new dependencies.  Used
	 * only for [...[X]...]->X links. */
	F_READ             = (1 << 3),  

	/* ($[...]) Content of file is used as variable */ 
	F_VARIABLE         = (1 << 4),

	/* Used only in Link.flags in the seoncd pass.  Not used for
	 * dependencies.  Means to override all trivial flags. */ 
	F_OVERRIDETRIVIAL        = (1 << 5),
};

/* Number of flags that are used, i.e., are transitive.  They correspond
 * to the first N flags declared in Flag */ 
const int F_COUNT= 3;

/* Total count */
const int F_ALL=  6;

const char *const flags_chars= "!?&`$*"; 

/* Textual representation of a flags value. 
 */
string format_flags(Flags flags) 
{
	string ret= "";
	for (int i= 0;  i < F_ALL;  ++i)
		if (flags & (1 << i))  ret += flags_chars[i]; 
	return ret;
}

/* A dependency, which can be simple or compound.  All dependencies
 * carry information about their place(s) of declaration. 
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

	virtual const Place &get_place_existence() const= 0;
	virtual const Place &get_place_optional () const= 0; 
	virtual const Place &get_place_trivial  () const= 0;

	virtual void set_place_existence(const Place &place)= 0;
	virtual void set_place_optional (const Place &place)= 0;
	virtual void set_place_trivial  (const Place &place)= 0;

	virtual string format() const= 0; 

#ifndef NDEBUG
	virtual void print() const= 0; 
#endif
};

class Base_Dependency
	:  public Dependency
{
public:

	Flags flags;

	Place place_existence;
	Place place_optional; 
	Place place_trivial;

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

	const Place &get_place_existence() const {
		return place_existence; 
	}

	const Place &get_place_optional() const {
		return place_optional; 
	}

	const Place &get_place_trivial() const {
		return place_trivial; 
	}

	void set_place_existence(const Place &place_) {
		place_existence= place_;
	}

	void set_place_optional(const Place &place_) {
		place_optional= place_; 
	}

	void set_place_trivial(const Place &place_) {
		place_trivial= place_; 
	}
};

/* A parametrized dependency denoting an individual target name.  Does
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
	
	/* Take the dependency place from the target place */ 
	Direct_Dependency(Flags flags_,
			  const Place_Param_Target &place_param_target_)
		:  Base_Dependency(flags_),
		   place_param_target(place_param_target_),
		   place(place_param_target_.place) 
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

	const Place &get_place() const {
		return place; 
	}

	shared_ptr <Dependency> instantiate(const map <string, string> &mapping) const;

	bool is_unparametrized() const {
		return place_param_target.place_param_name.get_n() == 0; 
	}

	void check() const {
		assert(place_param_target.type != T_ROOT); 
		/* Must not be dynamic, since dynamic dependencies are
		 * represented using Dynamic_Dependency */ 
		assert(place_param_target.type < T_DYNAMIC);
	}

	string format() const {
		string text_param_target= place_param_target.format(); 
		string text_flags= format_flags(flags);
		return fmt("%s%s",
			   text_flags,
			   text_param_target);
	}

#ifndef NDEBUG
	void print() const {
		place.print_beginning(); 
		string text= place_param_target.format();
		fprintf(stderr, "%d %s\n", flags, text.c_str()); 
	}
#endif
};

/* A dynamic dependency. 
 */
class Dynamic_Dependency
	:  public Base_Dependency
{
public:
	
	shared_ptr <Dependency> dependency;

	Dynamic_Dependency(Flags flags_,
			   shared_ptr <Dependency> dependency_)
		:  Base_Dependency(flags_), 
		   dependency(dependency_)
	{
		assert((flags & F_READ) == 0); 
	}

	shared_ptr <Dependency> 
	instantiate(const map <string, string> &mapping) const
	{
		return shared_ptr <Dependency> 
			(new Dynamic_Dependency
			 (flags, 
			  dependency->instantiate(mapping)));
	}

	bool is_unparametrized() const
	{
		return dependency->is_unparametrized(); 
	}

	const Place &get_place() const {
		return dependency->get_place(); 
	}

	string format() const {
		string text_dependency= dependency->format(); 
		string text_flags= format_flags(flags);
		return fmt("%s[%s]",
			   text_flags,
			   text_dependency); 
	}

#ifndef NDEBUG
	void print() const {
		fprintf(stderr, "dynamic %d of:  ", flags);
		dependency->print(); 
	}
#endif
};

/* A stack of dependency bits. (Only transitive bits).
 * Lower bits denote relationships lower in the hierarchy.  The depth K
 * is the number of times the link is dynamic.  (K+1) bits are actually
 * stored for each flag.  The maximum depth K is therefore CHAR_BITS *
 * sizeof(int) - 1, i.e., at least 15 on standard C platforms, and 31 on
 * almost all used platforms.  
 */
/* As a general rule, indexes named I go over the F_COUNT different
 * flags (0..F_COUNT-1), and indexes named J go over the (K+1) levels of
 * depth (0..K).  
 */
/* Example:  a dynamic dependency  ?[!X]  would be represented by the stack of bits
 *   J=1:    bit ?
 *   J=0:    bit !
 */
class Stack
{
private:
	unsigned k;
	unsigned bits[F_COUNT];

public:
	/* Check the internal consistency of this object */ 
	void check() const {
		assert(k + 1 < CHAR_BIT * sizeof(int)); 
		for (int i= 0;  i < F_COUNT;  ++i) {
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
		for (int i= 0;  i < F_COUNT;  ++i) {
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
		for (int i= 0;  i < F_COUNT;  ++i) {
			ret |= ((bits[i] & 1) << i);
		}
		return ret;
	}

	Flags get_highest() const {
		check(); 
		Flags ret= 0;
		for (int i= 0;  i < F_COUNT;  ++i) {
			ret |= (((bits[i] >> k) & 1) << i);
		}
		return ret;
	}

	/* Get the flags when K = 0. 
	 */
	Flags get_one() const {
		assert(k == 0);
		check();
		Flags ret= 0;
		for (int i= 0;  i < F_COUNT;  ++i) {
			ret |= bits[i] << i;
		}
		return ret;
	}

	Flags get(int j) const {
		Flags ret= 0;
		for (int i= 0;  i < F_COUNT;  ++i) {
			ret |= ((bits[i] >> j) & 1) << i;
		}
		return ret;
	}

	void add(Stack stack_) {
		check(); 
		assert(stack_.get_k() == this->get_k()); 
		for (int i= 0;  i < F_COUNT;  ++i) {
			this->bits[i] |= stack_.bits[i]; 
		}
	}

	/* Add the negation of the argument */ 
	void add_neg(Stack stack_) {
		check(); 
		assert(stack_.get_k() == this->get_k()); 
		for (int i= 0;  i < F_COUNT;  ++i) {
			this->bits[i] |= ((1 << (k+1)) - 1) ^ stack_.bits[i]; 
		}
		check(); 
	}

	void add_lowest(Flags flags) {
		check();
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] |= ((flags & (1 << i)) != 0);
		}
	}

	void add_highest(Flags flags) {
		check();
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] |= ((flags & (1 << i)) != 0) << k;
		}
	}

	void rem_highest(Flags flags) {
		check(); 
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] &= ~(((flags & (1 << i)) != 0) << k);
		}
	}

	void add_highest_neg(Flags flags) {
		check();
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] |= ((flags & (1 << i)) == 0) << k;
		}
	}

	/* K must be zero */ 
	void add_one_neg(Flags flags) {
		assert(k == 0);
		check();
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] |= ((flags >> i) & 1) ^ 1;
		}
	}

	/* K must be zero */ 
	void add_one_neg(Stack stack_) {
		assert(this->k == 0);
		assert(stack_.k == 0);
		check();
		for (int i= 0;  i < F_COUNT;  ++i) {
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
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] <<= 1;
		}
	}

	/* Remove the lowest level. (In-place change) */
	void pop() {
		assert(k > 0); 
		--k;
		for (int i= 0;  i < F_COUNT;  ++i) {
			bits[i] >>= 1;
		}
	}

	string format() const {
		string ret= "";
		for (int j= k;  j >= 0;  --j) {
			Flags flags_j= get(j);
			ret += format_flags(flags_j);
			if (j)  ret += ',';
		}
		return fmt("{%s}", ret); 
	}
};

Dependency::~Dependency() { }

shared_ptr <Dependency> Direct_Dependency
::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dependency> ret(new Direct_Dependency(flags, *ret_target, place));

	assert(ret_target->place_param_name.get_n() == 0); 
	string name= ret_target->place_param_name.format_mid(); 

	if ((flags & F_VARIABLE) &&
		name.find('=') != string::npos) {

		assert(ret_target->type == T_FILE); 
		
		place <<
			fmt("dynamic variable $[%s] cannot be instantiated"
			    " with parameter value that contains '='", 
			    name);
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
