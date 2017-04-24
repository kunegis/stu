#ifndef TYPE_HH
#define TYPE_HH

class Type
/* 
 * The type of a target.  A target can:
 *  - Be a file or a transient
 *  - Have any dynamicity level, including node
 */
{
public:

	static const Type TRANSIENT, FILE, DYNAMIC_TRANSIENT, DYNAMIC_FILE;
	/* Predefined values for the four smallest cases */ 

	bool is_dynamic() const {
		return value >= T_DYNAMIC_TRANSIENT;
	}

	unsigned get_depth() const 
	/* The level of dynamicity, i.e., corresponding to the number of
	 * bracket pairs in the syntax.  Zero for non-dynamic targets.  */
	{
		return value >> 1;
	}

	bool is_any_transient() const {
		return ! (value & 1);
	}

	bool is_any_file() const {
		return value & 1;
	}

	Type get_base() const {
		return Type(value & 1); 
	}

	bool operator == (const Type &type) const {
		return this->value == type.value;
	}

	bool operator == (unsigned value_) const {
		return this->value == value_; 
	}

	bool operator != (const Type &type) const {
		return this->value != type.value;
	}

	bool operator != (unsigned value_) const {
		return this->value != value_; 
	}

	int operator - (const Type &type) const {
		/* We can only subtract compatible types */ 
		assert(((this->value ^ type.value) & 1) == 0);

		return (this->value - type.value) / 2; 
	}

	Type operator - (int depth) const {
		assert((int)(value >> 1) - depth >= 0); 
		return Type(value - 2 * depth);
	}

	Type &operator ++ () {
		value += 2;
		return *this; 
	}

	Type &operator -- () {
		assert(value >= T_DYNAMIC_TRANSIENT);
		value -= 2;
		return *this;
	}

	Type &operator += (int depth) {
		assert(depth >= 0 || value > value + 2 * depth); 
		value += 2 * depth;
		return *this;
	}

	size_t get_value_for_hash() const {
		return hash <unsigned> ()(value); 
	}

	bool operator < (const Type &type) const {
		return this->value < type.value; 
	}

private:  

	unsigned value;
	/*
	 * LSB                        MSB
	 * +-----+-------------...------+
	 * |  F  | dynamicity           | 
	 * +-----+-------------...------+ 
	 *
	 *    F    whether this is a transient (0) or a file (1) 
	 */

	Type(unsigned value_)
		:  value(value_)
	{  }

	enum: unsigned {
		T_TRANSIENT         = 0,
		/* A transient target */ 
	
		T_FILE              = 1,
		/* A file in the file system; this entry has to come before
		 * T_DYNAMIC because it counts also as a dynamic dependency of
		 * depth zero. */

		T_DYNAMIC_TRANSIENT = 2,
		/* A dynamic transient target -- only used for the Target object of
		 * executions */

		T_DYNAMIC_FILE      = 3
		/* A dynamic target -- only used for the Target object of executions */   

		/* Larger values denote multiply dynamic targets.  They are only
		 * used as the target of Execution objects.  */
	};
};

const Type Type::TRANSIENT(Type::T_TRANSIENT);
const Type Type::FILE(Type::T_FILE);
const Type Type::DYNAMIC_TRANSIENT(Type::T_DYNAMIC_TRANSIENT);
const Type Type::DYNAMIC_FILE(Type::T_DYNAMIC_FILE);

#endif /* ! TYPE_HH */
