#include "done.hh"

bool Done::is_done_from_flags(Flags flags) const
{
	Done d= from_flags(flags);
	return (~bits & d.bits) == 0;
}

Done Done::from_flags(Flags flags)
{
	static_assert(F_PERSISTENT == 1 << 0);
	static_assert(F_OPTIONAL   == 1 << 1);
	static_assert(F_TRIVIAL    == 1 << 2);
	return Done(
		(~flags & (F_PERSISTENT | F_OPTIONAL))
		* (1 | (flags & F_PHASE_B ? F_TRIVIAL : 0)));
}

Done Done::from_flags_trivial_and_nontrivial(Flags flags)
{
	static_assert(F_PERSISTENT == 1 << 0);
	static_assert(F_OPTIONAL   == 1 << 1);
	static_assert(F_TRIVIAL    == 1 << 2);
	return Done(
		(~flags & (F_PERSISTENT | F_OPTIONAL))
		* (1 | F_TRIVIAL));
}

#ifndef NDEBUG

string Done::show() const
{
	char ret[7]= "[0000]";
	for (int i= 0; i < 4; ++i)
		ret[1+i] |= 1 & bits >> i;
	return ret;
}

#endif /* ! NDEBUG */
