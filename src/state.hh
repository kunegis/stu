#ifndef STATE_HH
#define STATE_HH

#include "show.hh"

/*
 * These are set for individual executors.  The semantics of each is chosen such that in a
 * new executor object, the value is zero.  The semantics of the different bits are
 * distinct and could just as well be realized as individual "bool" variables.
 */

class State
{
public:
	using Type = unsigned;

	static constexpr Type NEED_BUILD = 1 << 0;
	/* Whether this target needs to be built.  When a target is finished, this value
	 * is propagated to the parent executors. */

	static constexpr Type CHECKED = 1 << 1;
	/* Whether file targets have been checked. */

	static constexpr Type EXISTING = 1 << 2;
	/* All file targets are known to exist.  Only in File_Executor.  May be set when
	 * there are no file targets. */

	static constexpr Type MISSING = 1 << 3;
	/* At least one file target is known not to exist (only possible if there is at
	 * least one file target in File_Executor). */

	constexpr static int COUNT = 4;

	State(): bits(0) {}

	bool need_build() const { return bits & NEED_BUILD; }
	void set_need_build() { bits |= NEED_BUILD; }
	void reset_missing() { bits &= ~MISSING; }
	void set_existing() { bits |= EXISTING; bits &= ~MISSING; }
	bool missing() const { return bits & MISSING; }
	void set_missing() { bits |= MISSING; bits &= ~EXISTING; }
	void set_unknown() { bits &= ~(MISSING | EXISTING); }
	bool checked() const { return bits & CHECKED; }
	void set_checked() { bits |= CHECKED | EXISTING; bits &= ~MISSING; }

private:
	Type bits;
	constexpr State(Type b): bits(b) {}

#ifndef NDEBUG
	friend void render(State, Parts &, Rendering);
#endif /* ! NDEBUG */
};

#ifndef NDEBUG
void render(State, Parts &, Rendering= 0);
#endif /* ! NDEBUG */

#endif /* ! STATE_HH */
