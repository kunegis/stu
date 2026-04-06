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
	static const State NEED_BUILD;
	/* Whether this target needs to be built.  When a target is finished, this value
	 * is propagated to the parent executors. */

	static const State CHECKED;
	/* Whether a certain check has been performed.  Only used by File_Executor. */

	static const State EXISTING;
	/* All file targets are known to exist.  Only in File_Executor.  May be set when
	 * there are no file targets. */

	static const State MISSING;
	/* At least one file target is known not to exist (only possible if there is at
	 * least one file target in File_Executor). */

	constexpr static int COUNT = 4;

	State(): bits(0) {}

	State operator&(State b) const { return State(bits & b.bits); }
	State operator|(State b) const { return State(bits | b.bits); }
	State operator~() const { return State(~bits); }
	State &operator&=(State b) { bits &= b.bits; return *this; }
	State &operator|=(State b) { bits |= b.bits; return *this; }
	operator bool() const { return bits != 0; }

private:
	using Type = unsigned;
	Type bits;
	constexpr State(Type b): bits(b) {}
	friend void render(State, Parts &, Rendering);
};

inline constexpr State State::NEED_BUILD(1 << 0);
inline constexpr State State::CHECKED   (1 << 1);
inline constexpr State State::EXISTING  (1 << 2);
inline constexpr State State::MISSING   (1 << 4);

#ifndef NDEBUG
void render(State, Parts &, Rendering= 0);
#endif /* ! NDEBUG */

#endif /* ! STATE_HH */
