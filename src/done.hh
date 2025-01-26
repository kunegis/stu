#ifndef DONE_HH
#define DONE_HH

/*
 * Denotes which aspects of an execution have been done.  Each bit that is set
 * represents one aspect that was done.  When an executor is invoked with a
 * certain set of flags, all flags *not* passed will be set when the execution
 * is finished.  This is a different way to encode the three placed flags.  The
 * first two flags correspond to the first two flags (persistent and optional).
 * These two are duplicated in order to accommodate trivial dependencies.
 */

#include "flags.hh"

class Done
{
public:
	static constexpr unsigned D_NONPERSISTENT_TRIVIAL         = 1 << 0;
	static constexpr unsigned D_NONOPTIONAL_TRIVIAL           = 1 << 1;
	static constexpr unsigned D_NONPERSISTENT_NONTRIVIAL      = 1 << 2;
	static constexpr unsigned D_NONOPTIONAL_NONTRIVIAL        = 1 << 3;

	static constexpr unsigned D_ALL                           = (1 << 4) - 1;
	static constexpr unsigned D_ALL_OPTIONAL=
		D_NONPERSISTENT_TRIVIAL | D_NONPERSISTENT_NONTRIVIAL;

	Done(unsigned b): bits(b) { }
	Done(): bits(0) { }

	Done &operator|=(Done d) { bits |= d.bits; return *this; }
	bool is_all() const { return (~bits & D_ALL) == 0; }
	bool is_done_from_flags(Flags flags) const;
	void set_all() { bits= ~0; }

	static Done from_flags(Flags flags);

#ifndef NDEBUG
	string show() const;
#endif

private:
	unsigned bits;
};

#endif /* ! DONE_HH */
