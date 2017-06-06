#ifndef TYPE_HH
#define TYPE_HH

class Type
/* 
 * The type of a target.  A target can:
 *  - Be a file or a transient
 *  - Have any dynamicity level, including node
 */
// TODO deprecate the dynamic part of this.  It now only needs to store
// the file/transient distinction.  
// TODO maybe even deprecate it completely.  We only need the FILE and
// TRANSIENT bits, so get back to a int representation.  I.e., make the
// individual bits defined below be in the global namespace. 
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
		return value >> 2;
	}

	bool is_any_transient() const {
		return ! (value & 2);
	}

	bool is_any_file() const {
		return value & 2;
	}

	Type get_base() const {
		return Type(value & 2); 
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
		assert(((this->value ^ type.value) & 2) == 0);

		return (this->value - type.value) / 4; 
	}

	Type operator - (int depth) const {
		assert((int)(value >> 2) - depth >= 0); 
		return Type(value - 4 * depth);
	}

	Type &operator ++ () {
		value += 4;
		return *this; 
	}

	Type &operator -- () {
		assert(value >= T_DYNAMIC_TRANSIENT);
		value -= 4;
		return *this;
	}

	Type &operator += (int depth) {
		assert(depth >= 0 || value > value + 4 * depth); 
		value += 4 * depth;
		return *this;
	}

	unsigned get_value() const {  return value;  }

	size_t get_value_for_hash() const {
		return hash <unsigned> ()(value); 
	}

	bool operator < (const Type &type) const {
		return this->value < type.value; 
	}

	enum: unsigned {

		T_TRANSIENT         = 0,
		/* A transient target */ 
	
		T_FILE              = 2,
		/* A file in the file system; this entry has to come before
		 * T_DYNAMIC because it counts also as a dynamic dependency of
		 * depth zero. */

		T_MASK_FILE_TRANSIENT = 2,

		T_DYNAMIC_TRANSIENT = 4,
		/* A dynamic transient target -- only used for the Target object of
		 * executions */

		T_DYNAMIC_FILE      = 6
		/* A dynamic target -- only used for the Target object of executions */   

		/* Larger values denote multiply dynamic targets.  They are only
		 * used as the target of Execution objects.  */
	};

private:  

	unsigned value;
	/*
	 * LSB                        MSB
	 * +-----+-----+-------------...------+
	 * |  R  |  F  | dynamicity           | 
	 * +-----+-----+-------------...------+ 
	 *
	 *    R    Reserved for the purpose of Target2, always zero in
	 *         normal used, but used as extra value sometimes
	 *    F    whether this is a transient (0) or a file (1) 
	 *
	 * Due to the reserved bit, all values are even. 
	 */

	Type(unsigned value_)
		:  value(value_)
	{  }
};

const Type Type::TRANSIENT(Type::T_TRANSIENT);
const Type Type::FILE(Type::T_FILE);
const Type Type::DYNAMIC_TRANSIENT(Type::T_DYNAMIC_TRANSIENT);
const Type Type::DYNAMIC_FILE(Type::T_DYNAMIC_FILE);

#endif /* ! TYPE_HH */
