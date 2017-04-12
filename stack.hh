#ifndef STACK_HH
#define STACK_HH

class Stack
/*
 * A stack of dependency bits.  Contains only placed bits.  Lower
 * bits denote relationships lower in the hierarchy.  The DEPTH is the
 * number of times the link is dynamic.  (DEPTH+1) bits are stored for
 * each flag.  The maximum DEPTH is therefore CHAR_BITS * sizeof(int) -
 * 1, i.e., at least 15 on standard C platforms, and 31 on almost all
 * used platforms.
 *
 * As a general rule, indexes named I go over the C_TRANSITIVE different
 * flags (0..C_TRANSITIVE-1), and indexes named J go over the (DEPTH+1)
 * levels of depth (0..DEPTH).
 *
 * Example:  a dynamic dependency  -o [ -p X]  would be represented by
 * the stack of bits 
 *   J=1:    bit 'o'
 *   J=0:    bit 'p'
 *
 * Objects of type Stack are small enough to be passed by value. 
 */
{
public:
	Stack()
		/* Depth is zero, the single flag is zero */ 
		:  depth(0)
	{
		memset(bits, 0, sizeof(bits));
		check();
	}

	explicit Stack(Flags flags)
		/* Depth is zero, the flag type is given */ 
		:  depth(0)
	{
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i]= ((flags & (1 << i)) != 0);
		}
		check(); 
	}

	Stack(unsigned depth_, int zero) 
		/* Initalize to all-zero with the given depth K_ */ 
		:  depth(depth_)
	{
		(void) zero; 
		if (depth >= CHAR_BIT * sizeof(int) - 1) {
			print_error("dynamic dependency recursion limit exceeded");
			throw ERROR_FATAL; 
		}
		memset(bits, 0, sizeof(bits));
		check();
	}

	Stack(shared_ptr <Dependency> dependency);

	void check() const 
	/* Check the internal consistency of this object */ 
	{
		assert(depth + 1 < CHAR_BIT * sizeof(int)); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			/* Only the (K+1) first bits may be set */ 
			assert((bits[i] & ~((1 << (depth+1)) - 1)) == 0); 
		}
	}

	unsigned get_depth() const 
	{
		return depth;
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
			ret |= (((bits[i] >> depth) & 1) << i);
		}
		return ret;
	}

	Flags get_one() const 
	/* Get the flags when K == 0 */
	{
		assert(depth == 0);
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
		assert(stack_.get_depth() == this->get_depth()); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= stack_.bits[i]; 
		}
	}

	void add_neg(Stack stack_) 
	/* Add the negation of the argument */ 
	{
		check(); 
		assert(stack_.get_depth() == this->get_depth()); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= ((1 << (depth+1)) - 1) ^ stack_.bits[i]; 
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
			bits[i] |= ((flags & (1 << i)) != 0) << depth;
		}
	}

	void rem_highest(Flags flags) {
		check(); 
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] &= ~(((flags & (1 << i)) != 0) << depth);
		}
	}

	void add_highest_neg(Flags flags) {
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags & (1 << i)) == 0) << depth;
		}
	}

	void add_one_neg(Flags flags) 
	/* Add the negation of FLAGS to the only level when K is zero.  */  
	{
		assert(depth == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] |= ((flags >> i) & 1) ^ 1;
		}
	}

	void add_one_neg(Stack stack_) 
	/* K must be zero */ 
	{
		assert(this->depth == 0);
		assert(stack_.depth == 0);
		check();
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			this->bits[i] |= stack_.bits[i] ^ 1;
		}
	}

	void push() 
	/* Add a lowest level. (In-place change) */ 
	{
		assert(depth < CHAR_BIT * sizeof(int)); 
		if (depth == CHAR_BIT * sizeof(int) - 2) {
			print_error("dynamic dependency recursion limit exceeded");
			throw ERROR_FATAL; 
		}
		++depth;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] <<= 1;
		}
	}

	void pop() 
	/* Remove the lowest level. (In-place change) */
	{
		assert(depth > 0); 
		--depth;
		for (int i= 0;  i < C_TRANSITIVE;  ++i) {
			bits[i] >>= 1;
		}
	}

	string format() const {
		string ret= "";
		for (int j= depth;  j >= 0;  --j) {
			Flags flags_j= get(j);
			ret += flags_format(flags_j);
			if (j)  ret += ',';
		}
		return fmt("{%s}", ret); 
	}

private:

	unsigned depth;
	/* The depth */

	unsigned bits[C_TRANSITIVE];
	/* The bits */ 
};

Stack::Stack(shared_ptr <Dependency> dependency) 
{
	assert(dependency->is_normalized()); 

	depth= 0;
	memset(bits, 0, sizeof(bits));

	while (dynamic_pointer_cast <Dynamic_Dependency> (dependency)) {
		shared_ptr <Dynamic_Dependency> dynamic_dependency
			= dynamic_pointer_cast <Dynamic_Dependency> (dependency);
		add_lowest(dynamic_dependency->flags);

		push(); 
		dependency= dynamic_dependency->dependency; 
	}

	add_lowest(dependency->get_flags()); 
}

#endif /* ! STACK_HH */
