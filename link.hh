#ifndef LINK_HH
#define LINK_HH

/* Information about one parent--child link. 
 */ 
class Link
{
private:
	void check() const {
		avoid.check();
		if (avoid.get_k() == 0) {
			assert(avoid.get_lowest() == (flags & ((1 << F_COUNT) - 1)));
		}
	}

public:

	/* The length is one plus the dynamicity (number of dynamic
	 * indirections).  Small indices indicate the links lower in the
	 * hierarchy. 
	 * This variable only needs to hold the EXISTENCE and OPTIONAL
	 * bits (i.e., transitive bits). 
	 */
	Stack avoid;

	/* Flags that are valid for this dependency */ 
	Flags flags;

	/* The place of the declaration of the dependency */ 
	Place place; 

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

	/* The flags of the Dependency_Info only contain the top-level flags
	 * of the dependency if this is a direct dependency. 
	 */ 
	Link(shared_ptr <Dependency> dependency_)
		:  avoid(dependency_),
		   flags(dynamic_pointer_cast <Dynamic_Dependency> (dependency_)
			 ? (dependency_->get_flags() & ~((1 << F_COUNT) - 1))
			 : dependency_->get_flags()),
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
};

#endif /* ! LINK_HH */
