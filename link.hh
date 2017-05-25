#ifndef LINK_HH
#define LINK_HH

/* 
 * Information about one parent--child link. 
 */ 
class Link
{
public:

//	Stack avoid;
//	/* Negation of flags valid for each level.  The depth equals the
//	 * depth of the dependency.  This variable only holds the
//	 * PERSISTENT/OPTIONAL/TRIVIAL bits (i.e., transitive bits as
//	 * defined by F_COUNT).  */
//	// TODO rename the 'avoid' variables to 'flag_stack' or similar,
//	// to emphasize that they contain flags, and make clear that
//	// they are not inverted with respected to flags.  At best, only
//	// the 'done' variables should be called 'avoid'. 

	Flags flags;
	/* Flags that are valid for this dependency, including the
	 * non-transitive ones. */
	
	Place place; 
	/* The place of the declaration of the dependency for purposes
	 * of printing error messages.  Empty for 
	 * A->[...[A]...] links and other links which are not printed */ 

	shared_ptr <Dependency> dependency;
	/* This is null for the root target.  May contain less flags
	 * than stored in FLAGS, if additional flags came not from the
	 * dependency but from other sources.  Always a normalized
	 * dependency.  */

	Link() { }

	Link(//Stack avoid_,
	     Flags flags_,
	     Place place_,
	     shared_ptr <Dependency> dependency_)
		:  //avoid(avoid_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check(); 
	}

	Link(shared_ptr <Dependency> dependency_,
	     Flags flags_,
	     const Place &place_)
		:  //avoid(dependency_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check();
	}

	/* The flags in this object only contain the top-level flags
	 * of the dependency if this is a dynamic dependency.  */ 
	Link(shared_ptr <Dependency> dependency_)
		:  //avoid(dependency_),
		   flags(dependency_->get_flags()),
		   place(dependency_->get_place()),
		   dependency(dependency_)
	{ 
		check(); 
	}

	void add(
		 // TODO make it obsolete as the operation is too simple
//		 Stack avoid_, 
		 Flags flags_) {
//		assert(avoid.get_depth() == avoid_.get_depth());
//		avoid.add(avoid_);
		flags |= flags_;
		check(); 
	}

	string format_out() const;

private:

#ifndef NDEBUG
	void check() const;
#else   
	void check() const {  }
#endif
};

string Link::format_out() const 
{
	string text_dependency= 
		dependency == nullptr 
		? "NULL"
		: dependency->format_out(); 

	string text_flags= flags_format(flags);
//	string text_avoid= avoid.format();

	if (text_flags != "")  text_flags= ", " + text_flags;
//	if (text_avoid != "")  text_avoid= ", " + text_avoid; 

	return fmt("Link(%s%s)",
		   text_dependency,
		   text_flags//, 
//		   text_avoid
		   ); 
}

#ifndef NDEBUG
void Link::check() const 
{
//	avoid.check();

	assert(dependency == nullptr || dependency->is_normalized()); 
		
	/* Check that the highest level in AVOID equals the
	 * TRANSITIVE flags in FLAGS */ 
//	assert(avoid.get_highest() == (flags & ((1 << C_TRANSITIVE) - 1)));

	/* Check that the flags correspond to the flags in DEPENDENCY */
	assert(dependency == nullptr ||
	       (dependency->get_flags() & ~flags) == 0);
}
#endif /* ! NDEBUG */

#endif /* ! LINK_HH */
