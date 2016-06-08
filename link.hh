#ifndef LINK_HH
#define LINK_HH

/* Information about one parent--child link. 
 */ 
class Link
{
public:
	/* Negation of flags valid for each level. 
	 * The length is one plus the dynamicity (number of dynamic
	 * indirections).  Small indices indicate the links lower in the
	 * hierarchy. 
	 * This variable only holds the EXISTENCE/OPTIONAL/TRIVIAL
	 * bits (i.e., transitive bits as defined by F_COUNT). 
	 */
	Stack avoid;

	/* Flags that are valid for this dependency, including the
	 * non-transitive ones.  The transitive-bit part of this
	 * variable always equals the lowest level bits in AVOID. 
	 */  
	Flags flags;

	/* The place of the declaration of the dependency */ 
	Place place; 

	/* This is null for the root target. 
	 * May contain less flags than stored in AVOID and FLAGS. 
	 */
	shared_ptr <Dependency> dependency;

	Link() { }

	Link(Stack avoid_,
	     Flags flags_,
	     Place place_,
	     shared_ptr <Dependency> dependency_)
		:  avoid(avoid_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check(); 
	}

	Link(shared_ptr <Dependency> dependency_,
	     Flags flags_,
	     const Place &place_)
		:  avoid(dependency_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check();
	}

	/* The flags in this object only contain the top-level flags
	 * of the dependency if this is a dynamic dependency. 
	 */ 
	Link(shared_ptr <Dependency> dependency_)
		:  avoid(dependency_),
		   flags(dependency_->get_flags()),
		   place(dependency_->get_place()),
		   dependency(dependency_)
	{ 
		check(); 
	}

	void add(Stack avoid_, Flags flags_) {
		assert(avoid.get_k() == avoid_.get_k());
		avoid.add(avoid_);
		flags |= flags_;
		check(); 
	}

	string format() const;

private:

#ifndef NDEBUG
	void check() const;
#else   
	void check() const {  }
#endif
};

string Link::format() const {
	string text_dependency= 
		dependency == nullptr 
		? "NULL"
		: dependency->format_out(); 
	string text_avoid= avoid.format();
	string text_flags= flags_format(flags);
	return fmt("Link(%s, %s, %s)",
		   text_dependency,
		   text_flags, 
		   text_avoid); 
}

#ifndef NDEBUG
void Link::check() const {
	avoid.check();
		
	/* Check that the highest level in AVOID equals the
	 * TRANSITIVE flags in FLAGS */ 
	assert(avoid.get_highest() == (flags & ((1 << F_COUNT) - 1)));

	/* Check that the flags correspond to the flags in DEPENDENCY */
	assert(dependency == nullptr ||
	       (dependency->get_flags() & ~flags) == 0);
}
#endif /* ! NDEBUG */

#endif /* ! LINK_HH */
